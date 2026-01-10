#include "jpeg_compression.h"
#include <stdint.h>
#include <c7x.h>
#include <utils/mem/include/app_mem.h>
#include <math.h>

int32_t JpegCompression_RemoteServiceHandler(char *service_name, uint32_t cmd, void *prm, uint32_t prm_size, uint32_t flags)
{
    JPEG_COMPRESSION_DTO* packet = (JPEG_COMPRESSION_DTO*) prm;


    uint8_t *vec_r = (uint8_t *)(uintptr_t)appMemShared2TargetPtr(packet->phys_addr_r);
    uint8_t *vec_g = (uint8_t *)(uintptr_t)appMemShared2TargetPtr(packet->phys_addr_g);
    uint8_t *vec_b = (uint8_t *)(uintptr_t)appMemShared2TargetPtr(packet->phys_addr_b);
    uint8_t *vec_y = (uint8_t *)(uintptr_t)appMemShared2TargetPtr(packet->phys_addr_y_out);
    int8_t *vec_interm_buffer_1 = (int8_t *)(uintptr_t)appMemShared2TargetPtr(packet->phys_addr_intermediate_1);
    int16_t *vec_interm_buffer_2 = (int16_t *)(uintptr_t)appMemShared2TargetPtr(packet->phys_addr_intermediate_2);
    int16_t *vec_interm_buffer_3 = (int16_t *)(uintptr_t)appMemShared2TargetPtr(packet->phys_addr_intermediate_3);
    float   *vec_dct_buff = (float *)(uintptr_t)appMemShared2TargetPtr(packet->phys_addr_dct_buff);

    int total_pixels = packet->width * packet->height;

    // Cache invalidation
    appMemCacheInv(vec_r, total_pixels);
    appMemCacheInv(vec_g, total_pixels);
    appMemCacheInv(vec_b, total_pixels);

    #ifdef DEBUG_CYCLE_COUNT
        uint64_t start_time, y_time, segmentation_time, dct_time, quantization_time, zig_zag_time, encoding_time;
        start_time = __TSC;
    #endif

    // Color space transformation: RGB --> Y (vec_y now holds grayscale values)
    // Store Y into vec_interm_buffer_1
    rgb_to_y(vec_r, vec_g, vec_b, vec_interm_buffer_1, total_pixels);
    #ifdef DEBUG_CYCLE_COUNT
        y_time = __TSC;
    #endif

    // Calculate number of blocks
    uint32_t blocks_w = 0;
    uint32_t block_h = 0;

    // Take Y's from vec_interm_buffer_1 and segmentate it into 8 x 8 blocks, stored in row-major manner in vec_interm_buffer_2
    // vec_interm_buffer_2 is larger ecause it contains int16_t data. We cast it to uint8_t so we can use it in this step and the next one.
    image_to_blocks(vec_interm_buffer_1, packet->width, packet->height, &blocks_w, &block_h, (int8_t*)vec_interm_buffer_2);
    #ifdef DEBUG_CYCLE_COUNT
        segmentation_time = __TSC;
    #endif
    
    // Perform DCT on each block, take blocks from vec_interm_buffer_2 and correspondent DCT coeffs in vec_interm_buffer_1
    uint32_t total_blocks = block_h * blocks_w;
    uint32_t b = 0;
    for(b = 0; b < total_blocks; b++) {
        perform_dct_on_block((int8_t*)vec_interm_buffer_2 + (b * 64), vec_dct_buff + (b * 64));
    }
    #ifdef DEBUG_CYCLE_COUNT
        dct_time = __TSC;
    #endif

    for(b = 0; b < total_blocks; b++) {
        quantize_block(vec_dct_buff + (b * 64), vec_interm_buffer_2 + (b * 64));
    }
    #ifdef DEBUG_CYCLE_COUNT
        quantization_time = __TSC;
    #endif

    for(b = 0; b < total_blocks; b++) {
        zigzag_order(vec_interm_buffer_2 + (b * 64), vec_interm_buffer_3 + (b * 64));
    }
    #ifdef DEBUG_CYCLE_COUNT
        zig_zag_time = __TSC;
    #endif

    BitWriter bw;
    // write the result into vec_y
    bw.buffer = vec_y;
    bw.byte_pos = 0;
    bw.bit_pos = 0;
    bw.current = 0;

    int16_t prev_dc = 0;
    for(b = 0; b < total_blocks; b++) {
        prev_dc = encode_coefficients(vec_interm_buffer_3 + (b * 64), prev_dc, &bw);
    }

    // clean out the remaining byte from BW
    if (bw.bit_pos > 0) {
        bw_put_byte(&bw, bw.current);
    }

    // Write out output size so A72 can perform serialization
    packet->output_size = bw.byte_pos;
    #ifdef DEBUG_CYCLE_COUNT
        encoding_time = __TSC;
    #endif

    // CACHE WRITEBACK (After processing)
    // Push data from cache to DDR so A72 can read it. 
    appMemCacheWb(vec_y, total_pixels);
    appMemCacheWb(vec_interm_buffer_1, total_pixels);
    appMemCacheWb(vec_interm_buffer_2, total_pixels * 2);           // the second buffer is bigger
    appMemCacheWb(vec_interm_buffer_3, total_pixels * 2);
    appMemCacheWb(vec_dct_buff, total_pixels * 4);

    // Perform cycle calculation and print

    #ifdef DEBUG_CYCLE_COUNT
        uint64_t diff_rgb   = y_time - start_time;
        uint64_t diff_seg   = segmentation_time - y_time;
        uint64_t diff_dct   = dct_time - segmentation_time;
        uint64_t diff_quant = quantization_time - dct_time;
        uint64_t diff_zz    = zig_zag_time - quantization_time;
        uint64_t diff_enc   = encoding_time - zig_zag_time;
        uint64_t diff_total = encoding_time - start_time;
              
        static char log_buf[2048]; 
        int offset = 0;
        
        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "\n");
        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "======================================================================\n");
        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "| %-30s | %32s |\n", "JPEG COMPRESSION STAGE", "CYCLES");
        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "======================================================================\n");

        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "| %-30s | %32llu |\n", "RGB -> Y Conversion", diff_rgb);
        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "| %-30s | %32llu |\n", "Segmentation (8x8)", diff_seg);
        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "| %-30s | %32llu |\n", "DCT Transform", diff_dct);
        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "| %-30s | %32llu |\n", "Quantization", diff_quant);
        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "| %-30s | %32llu |\n", "ZigZag Reorder", diff_zz);
        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "| %-30s | %32llu |\n", "Huffman Encoding", diff_enc);

        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "----------------------------------------------------------------------\n");
        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "| %-30s | %32llu |\n", "TOTAL CYCLES", diff_total);
        offset += snprintf(log_buf + offset, sizeof(log_buf)-offset, "======================================================================\n\n");

        // Single log so racing condition doesn't scatter the lines across output
        appLogPrintf("%s", log_buf);

    #endif
    return 0;
}

int32_t JpegCompression_Init()
{
    int32_t status = -1;
    appLogPrintf("JPEG Compression Service: Init ... !!!");
    status = appRemoteServiceRegister(JPEG_COMPRESSION_REMOTE_SERVICE_NAME, JpegCompression_RemoteServiceHandler);
    if(status != 0)
    {
        appLogPrintf("JPEG Compression Service: ERROR: Unable to register remote service handler\n");
    }
    appLogPrintf("JPEG Compression Service: Init ... DONE!!! (service name: %s\n", JPEG_COMPRESSION_REMOTE_SERVICE_NAME);
    return status;
}


void rgb_to_y(uint8_t *r_ptr, uint8_t *g_ptr, uint8_t *b_ptr, int8_t *y_ptr, int num_pixels)
{
    int32_t num_vectors = num_pixels / 32;
    int32_t i; 

    uchar32 * restrict vec_r = (uchar32 *) r_ptr;
    uchar32 * restrict vec_g = (uchar32 *) g_ptr;
    uchar32 * restrict vec_b = (uchar32 *) b_ptr;
    char32 * restrict vec_y = (char32 *) y_ptr;

    short32 coeff_r = (short32) 77;
    short32 coeff_g = (short32) 150;
    short32 coeff_b = (short32) 29;
    
    short32 val_128 = (short32) 128;

    for (i = 0; i < num_vectors; i++) {
        uchar32 r_in = vec_r[i];
        uchar32 g_in = vec_g[i];
        uchar32 b_in = vec_b[i];

        short32 r_s = convert_short32(r_in);
        short32 g_s = convert_short32(g_in);
        short32 b_s = convert_short32(b_in);

        short32 y_temp = (r_s * coeff_r) + (g_s * coeff_g) + (b_s * coeff_b);

        y_temp = y_temp >> 8;
        y_temp = y_temp - val_128;              // center around zero because of DCTs

        vec_y[i] = convert_char32(y_temp);
    }
}

/*
* Manual calculation of DCT and FP math. This is not ideal, but we will worry about optimization later on... :)
*/
void perform_dct_on_block(int8_t *b_start, float *dct_coeffs) {
    int u, v, x, y;
    float sum, cu, cv;

    for(u = 0; u < 8; u++) {
        for(v = 0; v < 8; v++) {
            
            sum = 0.0f;
            for(x = 0; x < 8; x++) {
                for(y = 0; y < 8; y++) {
                    sum += b_start[y * 8 + x] * 
                           cosf(((2 * x + 1) * u * PI_VAL) / 16.0f) * 
                           cosf(((2 * y + 1) * v * PI_VAL) / 16.0f);
                }
            }

            cu = (u == 0) ? (1.0f / sqrtf(2.0f)) : 1.0f;
            cv = (v == 0) ? (1.0f / sqrtf(2.0f)) : 1.0f;

            dct_coeffs[v * 8 + u] = 0.25f * cu * cv * sum;
        }
    }

}

void image_to_blocks(int8_t *image_buffer, uint32_t width, uint32_t height, uint32_t *out_blocks_w, uint32_t *out_blocks_h, int8_t *out_blocks) {
    // Honor the C89 standard...
    uint32_t blocks_w, blocks_h;
    uint32_t by, bx, y, x;
    uint32_t img_x, img_y, block_index;

    blocks_w = (width + 7) / 8;             // ceiling division
    blocks_h = (height + 7) / 8;             

    *out_blocks_w = blocks_w;
    *out_blocks_h = blocks_h;

    for(by = 0; by < blocks_h; by++) {
        for(bx = 0; bx < blocks_w; bx++) {

            for(y = 0; y < 8; y++) {
                for(x = 0; x < 8; x++) {
                    img_x = bx * 8 + x < width ? bx * 8 + x : width - 1;
                    img_y = by * 8 + y < height ? by * 8 + y : height - 1;
                    block_index = (by * blocks_w + bx) * 64 + (y * 8 + x);
                    out_blocks[block_index] = image_buffer[img_y * width + img_x];
                }
            }
        }
    }

}

void quantize_block(float *dct_block, int16_t* out_quantized_block) {
    uint32_t i = 0;
    for(i = 0; i < 64; i++) {
        out_quantized_block[i] = (int16_t)roundf(dct_block[i] / std_lum_qt[i]);         // rounding to nearest integer
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
    uint32_t i = 0;

    for(i = 0; i < 64; i++) {
        output_block[i] = input_block[zigzag_map[i]];
    }
}

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
    int16_t temp = value;
    uint8_t nbits = 0;

    if(value < 0) {
        value = -value;
        temp--;             // transform 2's complement to 1's complement
    }

    // Calculate number of bits needed
    // Calculation is done by shifting right until value becomes 0
    // This is why we work with absolute value - we only care about magnitude here
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
