#include "jpeg_compression.h"
#include <math.h>
#include <c7x.h>

void quantize_block(float* restrict dct_block, int16_t* restrict out_quantized_block) {
    ASSERT_ALIGNED_64(dct_block);
    ASSERT_ALIGNED_64(out_quantized_block);

    uint8_t i = 0;
    float16* input = (float16*)dct_block;
    short16* out = (short16*)out_quantized_block;
    float16* dct_table = (float16*)std_lum_qt_recip;

    float16 zeros = (float16)0.0f;
    float16 plus_half = (float16)0.5f;
    float16 minus_half = (float16)-0.5f;

    #pragma MUST_ITERATE(4, 4, 4)
    #pragma UNROLL(2)
    for(i = 0; i < 4; i++) {
        float16 in = *(input + i);
        float16 dct_in = *(dct_table + i);
        float16 result = in * dct_in;

        // Use __vpred for storing an array of bools
        __vpred pred = __cmp_ge_pred(result, zeros);
        float16 offset = __select(pred, plus_half, minus_half);

        *(out + i) = __convert_short16(result + offset);
    }

}
