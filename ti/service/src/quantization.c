#include "jpeg_compression.h"
#include <math.h>
#include <c7x.h>

void quantize_block(float* restrict dct_block, int16_t* restrict out_quantized_block) {
    uint8_t i = 0;
    float8* input = (float8*)dct_block;
    short8* out = (short8*)out_quantized_block;
    float8* dct_table = (float8*)std_lum_qt_recip;

    float8 zeros = (float8)0.0f;
    float8 plus_half = (float8)0.5f;
    float8 minus_half = (float8)-0.5f;

    for(i = 0; i < 4; i++) {
        float8 in = *(input + i);
        float8 dct_in = *(dct_table + i);
        float8 result = in * dct_in;

        // Use __vpred for storing an array of bools
        __vpred pred = __cmp_ge_pred(result, zeros);
        float8 offset = __select(pred, plus_half, minus_half);

        *(out + i) = __convert_short8(result + offset);
    }

}
