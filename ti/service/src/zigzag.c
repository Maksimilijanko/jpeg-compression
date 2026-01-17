#include "jpeg_compression.h"


void zigzag_order(const int16_t *input_block, int16_t *output_block, uint8_t num_blocks) {
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
    uint32_t i = 0, b;

    for(b = 0; b < num_blocks; b++) {

        #pragma MUST_ITERATE(64, 64, 64)
        #pragma UNROLL(8)
        for(i = 0; i < 64; i++) {
            output_block[b*64 + i] = input_block[b*64 + zigzag_map[i]];
        }
    }
}
