#ifndef DCT_H
#define DCT_H

#include <stdint.h>

#define PI 3.14159265358979323846f

/* DCT function declarations */

/*
    * Splits the input image into 8x8 blocks.
    * Pads the image with edge pixels if necessary (clamp).
    * Returns a pointer to a 3D array: blocks[block_y][block_x][64]
    * Also outputs the number of blocks in width and height via out_blocks_w and out_blocks_h.
    * Blocks are stored in row-major order.
    */
float * image_to_blocks(float *image, uint32_t width, uint32_t height, uint32_t *out_blocks_w, uint32_t *out_blocks_h);

float * perform_dct(float *blocks, uint32_t blocks_w, uint32_t blocks_h);
float * perform_dct_one_block(float *block);
float * encode_coefficients(float *dct_block);


#endif