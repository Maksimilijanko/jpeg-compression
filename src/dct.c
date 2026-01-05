#include "dct.h"
#include <stdlib.h>
#include <math.h>


void center_around_zero(float* grayscale_values, uint32_t width, uint32_t height) {
    for (uint32_t i = 0; i < width * height; i++) {
        grayscale_values[i] -= 128.0f; 
    }
}

void image_to_blocks(float *image, uint32_t width, uint32_t height, uint32_t *out_blocks_w, uint32_t *out_blocks_h, float **out_blocks) {
    uint32_t blocks_w = (width + 7) / 8;             // ceiling division
    uint32_t blocks_h = (height + 7) / 8;             
    uint32_t total_blocks = blocks_w * blocks_h;

    *out_blocks = (float*)calloc(1, total_blocks * 64 * sizeof(float));

    *out_blocks_w = blocks_w;
    *out_blocks_h = blocks_h;

    for(uint32_t by = 0; by < blocks_h; by++) {
        for(uint32_t bx = 0; bx < blocks_w; bx++) {

            for(uint32_t y = 0; y < 8; y++) {
                for(uint32_t x = 0; x < 8; x++) {
                    uint32_t img_x = bx * 8 + x < width ? bx * 8 + x : width - 1;
                    uint32_t img_y = by * 8 + y < height ? by * 8 + y : height - 1;
                    uint32_t block_index = (by * blocks_w + bx) * 64 + (y * 8 + x);
                    (*out_blocks)[block_index] = image[img_y * width + img_x];
                }
            }
        }
    }

}

void perform_dct_one_block(float *block, float *out_dct_block) {
    for(int u = 0; u < 8; u++) {
        for(int v = 0; v < 8; v++) {
            
            float sum = 0.0f;
            for(int x = 0; x < 8; x++) {
                for(int y = 0; y < 8; y++) {
                    sum += block[y * 8 + x] * 
                           cosf(((2 * x + 1) * u * PI) / 16.0f) * 
                           cosf(((2 * y + 1) * v * PI) / 16.0f);
                }
            }

            float cu = (u == 0) ? (1.0f / sqrtf(2.0f)) : 1.0f;
            float cv = (v == 0) ? (1.0f / sqrtf(2.0f)) : 1.0f;

            out_dct_block[v * 8 + u] = 0.25f * cu * cv * sum;
        }
    }

}

void quantize_block(float *dct_block, int16_t* out_quantized_block) {
    for(int i = 0; i < 64; i++) {
        out_quantized_block[i] = (int16_t)roundf(dct_block[i] / std_lum_qt[i]);         // rounding to nearest integer
    }
}

void perform_dct(float *blocks, uint32_t blocks_w, uint32_t blocks_h, float *out_dct_blocks) {
    uint32_t total_blocks = blocks_w * blocks_h;

    /* Process each block */
    for(uint32_t b = 0; b < total_blocks; b++) {
        perform_dct_one_block(blocks + (b * 64), out_dct_blocks + (b * 64));
    }
}

void zigzag_order(const int16_t *input_block, int16_t *output_block) {
    /*
    * Instead of computing the zigzag order on the fly, we use a predefined mapping.
    */
    const uint8_t zigzag_map[64] = {
     0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
    };

    for(int i = 0; i < 64; i++) {
        output_block[i] = input_block[zigzag_map[i]];
    }
}

void bw_write(BitWriter *bw, uint32_t code, int length) {
    for (int i = length - 1; i >= 0; i--) {
        // Isolate the i-th bit from the code
        uint8_t bit = (code >> i) & 1;
        
        // Write it to the current byte in the BitWriter
        if (bit) {
            // Write 1 to the current position using shifting and OR
            bw->current |= (1 << (7 - bw->bit_pos));
        }
        
        // Move to the next bit position
        bw->bit_pos++;
        
        // Check if we have filled the current byte. If so, write it to the buffer, reset current byte and bit position.
        if (bw->bit_pos == 8) {
            bw_put_byte(bw, bw->current);           // delegate writing to bw_put_byte so byte stuffing is performed
            bw->current = 0;
            bw->bit_pos = 0;
        }
    }
}

void bw_put_byte(BitWriter *bw, uint8_t val) {
    bw->buffer[bw->byte_pos++] = val;

    if(val == 0xFF)
        bw->buffer[bw->byte_pos++] = 0x00;              // byte stuff so decoder can distinguish markers and payload
}

VLI get_vli(int16_t value) {
    VLI vli;
    int16_t temp = value;

    if(value < 0) {
        value = -value;
        temp--;             // transform 2's complement to 1's complement
    }

    // Calculate number of bits needed
    // Calculation is done by shifting right until value becomes 0
    // This is why we work with absolute value - we only care about magnitude here
    uint8_t nbits = 0;
    while (value > 0) {
        nbits++;
        value >>= 1;
    }

    vli.len = nbits;
    vli.bits = (temp & ((1 << nbits) - 1)); // Mask to get the lower nbits
    return vli;
}

int16_t encode_coefficients(int16_t *dct_block, int16_t prev_dc
                            , BitWriter* bw) {
    
    // Predictive DC encoding
    int16_t diff = dct_block[0] - prev_dc;
    VLI vli = get_vli(diff);
    
    // Huffman DC encoding
    HuffmanCode hc = huff_dc_lum[vli.len];
    bw_write(bw, hc.code, hc.len);             // write DC Huffman code into bitstream
    
    if (vli.len > 0) {
        bw_write(bw, vli.bits, vli.len);       // write DC VLI bits into bitstream
    }

    int zeros_count = 0;
    
    for (int i = 1; i < 64; i++) {              // Skip DC component, start from AC components
        int16_t val = dct_block[i];

        if (val == 0) {
            zeros_count++;
        } else {
            // Handle any preceding zeros
            while (zeros_count > 15) {
                // ZRL (Zero Run Length): 16 continuous zeros
                // Symbol is 0xF0
                // We perform Huffman encoding for ZRL
                bw_write(bw, huff_ac_lum[0xF0].code, huff_ac_lum[0xF0].len);
                zeros_count -= 16;
            }

            // Get VLI for the non-zero value
            vli = get_vli(val);
            
            // Build the symbol: (RUNLENGTH << 4) | SIZE
            uint8_t symbol = (zeros_count << 4) | vli.len;
            
            // Get Huffman code for the symbol
            bw_write(bw, huff_ac_lum[symbol].code, huff_ac_lum[symbol].len);
            
            // Write the actual bits of the value
            bw_write(bw, vli.bits, vli.len);
            
            zeros_count = 0; 
        }
    }

    // EOB handling
    if (zeros_count > 0) {
        // If there are trailing zeros, write EOB (symbol 0x00)
        bw_write(bw, huff_ac_lum[0x00].code, huff_ac_lum[0x00].len);
    }
        
    // if (bw.bit_pos > 0) {
    //     bw.buffer[bw.byte_pos++] = bw.current;      // Flush out the last byte if it has remaining bits
    // }
    
    return dct_block[0];                       // Return current DC for next block's prediction       
}
