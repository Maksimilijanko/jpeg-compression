#include "dct.h"
#include <stdlib.h>

float * image_to_blocks(float *image, uint32_t width, uint32_t height, uint32_t *out_blocks_w, uint32_t *out_blocks_h) {
    uint32_t blocks_w = (width + 7) / 8;             // ceiling division
    uint32_t blocks_h = (height + 7) / 8;             
    uint32_t total_blocks = blocks_w * blocks_h;

    *out_blocks_w = blocks_w;
    *out_blocks_h = blocks_h;

    float* blocks = (float*)malloc(total_blocks * 8 * 8 * sizeof(float));

    for(int by = 0; by < blocks_h; by++) {
        for(int bx = 0; bx < blocks_w; bx++) {

            for(int y = 0; y < 8; y++) {
                for(int x = 0; x < 8; x++) {
                    uint32_t img_x = bx * 8 + x < width ? bx * 8 + x : width - 1;
                    uint32_t img_y = by * 8 + y < height ? by * 8 + y : height - 1;
                    uint32_t block_index = (by * blocks_w + bx) * 64 + (y * 8 + x);
                    blocks[block_index] = image[img_y * width + img_x];
                }
            }
        }
    }

    return blocks;
}