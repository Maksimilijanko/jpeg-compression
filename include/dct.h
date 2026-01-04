#ifndef DCT_H
#define DCT_H

#include <stdint.h>

/* DCT function declarations */

/*
    * Splits the input image into 8x8 blocks.
    * Pads the image with edge pixels if necessary (clamp).
    * Returns a pointer to a 3D array: blocks[block_y][block_x][64]
    * Also outputs the number of blocks in width and height via out_blocks_w and out_blocks_h.
    * Blocks are stored in row-major order.
    */
float * image_to_blocks(float *image, uint32_t width, uint32_t height, uint32_t *out_blocks_w, uint32_t *out_blocks_h);


#endif