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
    uint8_t *vec_interm_buffer_1 = (uint8_t *)(uintptr_t)appMemShared2TargetPtr(packet->phys_addr_intermediate_1);
    int16_t *vec_interm_buffer_2 = (int16_t *)(uintptr_t)appMemShared2TargetPtr(packet->phys_addr_intermediate_2);
    float   *vec_dct_buff = (float *)(uintptr_t)appMemShared2TargetPtr(packet->phys_addr_dct_buff);

    int total_pixels = packet->width * packet->height;

    // Cache invalidation
    appMemCacheInv(vec_r, total_pixels);
    appMemCacheInv(vec_g, total_pixels);
    appMemCacheInv(vec_b, total_pixels);

        
    // Color space transformation: RGB --> Y (vec_y now holds grayscale values)
    // Store Y into vec_interm_buffer_1
    rgb_to_y(vec_r, vec_g, vec_b, vec_interm_buffer_1, total_pixels);

    // Calculate number of blocks
    uint32_t blocks_w = 0;
    uint32_t block_h = 0;

    // Take Y's from vec_interm_buffer_1 and segmentate it into 8 x 8 blocks, stored in row-major manner in vec_interm_buffer_2
    // vec_interm_buffer_2 is larger ecause it contains int16_t data. We cast it to uint8_t so we can use it in this step and the next one.
    image_to_blocks(vec_interm_buffer_1, packet->width, packet->height, &blocks_w, &block_h, (uint8_t*)vec_interm_buffer_2);

    // Perform DCT on each block, take blocks from vec_interm_buffer_2 and correspondent DCT coeffs in vec_interm_buffer_1
    uint32_t total_blocks = block_h * blocks_w;
    uint32_t b = 0;
    for(b = 0; b < total_blocks; b++) {
        perform_dct_on_block((uint8_t*)vec_interm_buffer_2 + (b * 64), vec_dct_buff + (b * 64));
    }

    for(b = 0; b < total_blocks; b++) {
        quantize_block(vec_dct_buff + (b * 64), vec_interm_buffer_2 + (b * 64));
    }

    // CACHE WRITEBACK (After processing)
    // Push data from cache to DDR so A72 can read it. 
    appMemCacheWb(vec_y, total_pixels);
    appMemCacheWb(vec_interm_buffer_1, total_pixels);
    appMemCacheWb(vec_interm_buffer_2, total_pixels * 2);           // the second buffer is bigger
    appMemCacheWb(vec_dct_buff, total_pixels * 4); 
    

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


void rgb_to_y(uint8_t *r_ptr, uint8_t *g_ptr, uint8_t *b_ptr, uint8_t *y_ptr, int num_pixels)
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
void perform_dct_on_block(uint8_t *b_start, float *dct_coeffs) {
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

void image_to_blocks(uint8_t *image_buffer, uint32_t width, uint32_t height, uint32_t *out_blocks_w, uint32_t *out_blocks_h, uint8_t *out_blocks) {
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
