#include "dct.h"
#include <stdlib.h>
#include <math.h>

void center_around_zero(float* grayscale_values, uint32_t width, uint32_t height) {
    for (uint32_t i = 0; i < width * height; i++) {
        grayscale_values[i] -= 128.0f; 
    }
}


void image_to_blocks(float *image, uint32_t width, uint32_t height, uint32_t *out_blocks_w, uint32_t *out_blocks_h, float *out_blocks) {
    uint32_t blocks_w = (width + 7) / 8;             // ceiling division
    uint32_t blocks_h = (height + 7) / 8;             
    uint32_t total_blocks = blocks_w * blocks_h;

    *out_blocks_w = blocks_w;
    *out_blocks_h = blocks_h;

    for(int by = 0; by < blocks_h; by++) {
        for(int bx = 0; bx < blocks_w; bx++) {

            for(int y = 0; y < 8; y++) {
                for(int x = 0; x < 8; x++) {
                    uint32_t img_x = bx * 8 + x < width ? bx * 8 + x : width - 1;
                    uint32_t img_y = by * 8 + y < height ? by * 8 + y : height - 1;
                    uint32_t block_index = (by * blocks_w + bx) * 64 + (y * 8 + x);
                    out_blocks[block_index] = image[img_y * width + img_x];
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

void quantize_block(float *dct_block, const uint8_t *quant_table, int16_t* out_quantized_block) {
    for(int i = 0; i < 64; i++) {
        out_quantized_block[i] = (int16_t)roundf(dct_block[i] / quant_table[i]);         // rounting to nearest integer
    }

}