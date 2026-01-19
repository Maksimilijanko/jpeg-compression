#include "jpeg_compression.h"

void bw_write(BitWriter *bw, uint32_t code, int length) {
    int32_t i = 0;
    uint8_t bit = 0;
    
    for (i = length - 1; i >= 0; i--) {
        // Isolate the i-th bit from the code
        bit = (code >> i) & 1;
        
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
    
    int32_t abs_val = __abs(value);
    
    vli.len = 31 - __norm(abs_val);

    // temp is either going to be a positive value, or a negative value but in one's complement!
    int16_t temp = value + (value >> 15);

    // masking the temp variable to write out only the lower vli.len bits.
    vli.bits = temp & ((1 << vli.len) - 1);

    return vli;
}

int16_t encode_coefficients(int16_t* dct_block, int16_t prev_dc, BitWriter* bw) {

    uint32_t zeros_count = 0;
    uint32_t i = 0;
    uint8_t symbol;
    int16_t val;
    
    // Predictive DC encoding
    int16_t diff = dct_block[0] - prev_dc;
    VLI vli = get_vli(diff);
    
    // Huffman DC encoding
    HuffmanCode hc = huff_dc_lum[vli.len];
    bw_write(bw, hc.code, hc.len);             // write DC Huffman code into bitstream
    
    if (vli.len > 0) {
        bw_write(bw, vli.bits, vli.len);       // write DC VLI bits into bitstream
    }

    for (i = 1; i < 64; i++) {              // Skip DC component, start from AC components
        val = dct_block[i];

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
            symbol = (zeros_count << 4) | vli.len;
            
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
