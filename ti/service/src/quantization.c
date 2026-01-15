#include "jpeg_compression.h"
#include <math.h>

void quantize_block(float *dct_block, int16_t* out_quantized_block) {
    uint32_t i = 0;
    for(i = 0; i < 64; i++) {
        out_quantized_block[i] = (int16_t)roundf(dct_block[i] / std_lum_qt[i]);         // rounding to nearest integer
    }
}
