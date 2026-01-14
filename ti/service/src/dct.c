#include "jpeg_compression.h"
#include <stdint.h>
#include <c7x.h>
#include <utils/mem/include/app_mem.h>
#include <math.h>


/*
* This matmul avoid reduction ops and uses partial sums.
*/
void perform_dct_on_block(int8_t * restrict b_start, float * restrict dct_coeffs) {
    int i, k;
    float8 intermediate_regs[8];
 
    ASSERT_ALIGNED_64(dct_matrix_c);
    ASSERT_ALIGNED_64(dct_matrix_c_T);

    #ifdef DEBUG_CYCLE_COUNT
        start = __TSC;
    #endif

    #pragma MUST_ITERATE(8, 8, 8)
    for(i = 0; i < 8; i++) {
        //char8 in_char = *((char8 *)&b_start[i * 8]);
        //float8 in_vec = __convert_float8(in_char);

        float8 row_acc = (float8)0.0f;                                                          // acumulator

        #pragma UNROLL(8) 
        for(k = 0; k < 8; k++) {
            // float pixel_val = in_vec.s[k]; 
            float pixel_val = (float)b_start[i*8 + k];                                          
            
            float8 c_row = *((float8 *)&dct_matrix_c_T[k * 8]);                                 // for each pixel in a row we calculate it's impact on the output row
            
            row_acc += c_row * pixel_val;
        }

        intermediate_regs[i] = row_acc;
    }

    #ifdef DEBUG_CYCLE_COUNT
        timer1 = __TSC;
    #endif
    

    #pragma MUST_ITERATE(8, 8, 8)
    for(i = 0; i < 8; i++) {
        float8 c_factors = *((float8 *)&dct_matrix_c[i * 8]);
        
        float8 row_acc = (float8)0.0f;

        #pragma UNROLL(8)
        for(k = 0; k < 8; k++) {
            float c_val = c_factors.s[k];
            
            float8 m_row = intermediate_regs[k];
            
            row_acc += m_row * c_val;
        }

        *((float8 *)&dct_coeffs[i * 8]) = row_acc;
    }

    #ifdef DEBUG_CYCLE_COUNT
        timer2 = __TSC;
    #endif

}
