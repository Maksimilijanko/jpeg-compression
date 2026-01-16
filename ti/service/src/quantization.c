#include "jpeg_compression.h"
#include <math.h>
#include <c7x.h>

void quantize_block(float* restrict dct_block, int16_t* restrict out_quantized_block, int16_t num_blocks) {
    ASSERT_ALIGNED_64(dct_block);
    ASSERT_ALIGNED_64(out_quantized_block);

    uint8_t b = 0;
    float16* input = (float16*)dct_block;
    short16* out = (short16*)out_quantized_block;
    float16* dct_table = (float16*)std_lum_qt_recip;

    // Load QT table once
    float16 tbl_row0 = dct_table[0];
    float16 tbl_row1 = dct_table[1];
    float16 tbl_row2 = dct_table[2];
    float16 tbl_row3 = dct_table[3];

    float16 zeros = (float16)0.0f;
    float16 plus_half = (float16)0.5f;
    float16 minus_half = (float16)-0.5f;

    #pragma MUST_ITERATE(, , 2)
    #pragma UNROLL(2)
    for (b = 0; b < num_blocks; b++) {
        
        {
            float16 in = input[b*4 + 0];
            float16 res = in * tbl_row0;
            __vpred pred = __cmp_ge_pred(res, zeros);
            float16 off = __select(pred, plus_half, minus_half);
            out[b*4 + 0] = __convert_short16(res + off);
        }


        {
            float16 in = input[b*4 + 1];
            float16 res = in * tbl_row1;
            __vpred pred = __cmp_ge_pred(res, zeros);
            float16 off = __select(pred, plus_half, minus_half);
            out[b*4 + 1] = __convert_short16(res + off);
        }

        {
            float16 in = input[b*4 + 2];
            float16 res = in * tbl_row2; 
            __vpred pred = __cmp_ge_pred(res, zeros);
            float16 off = __select(pred, plus_half, minus_half);
            out[b*4 + 2] = __convert_short16(res + off);
        }

        {
            float16 in = input[b*4 + 3];
            float16 res = in * tbl_row3; 
            __vpred pred = __cmp_ge_pred(res, zeros);
            float16 off = __select(pred, plus_half, minus_half);
            out[b*4 + 3] = __convert_short16(res + off);
        }
    }

}
