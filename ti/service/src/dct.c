#include "jpeg_compression.h"
#include <stdint.h>
#include <c7x.h>
#include <utils/mem/include/app_mem.h>
#include <math.h>


/*
* This matmul avoid reduction ops and uses partial sums.
*/
void perform_dct_on_blocks(int8_t * restrict b_start, float * restrict dct_coeffs, int8_t num_blocks) {
    int i, k;
    uint8_t b;
 
    ASSERT_ALIGNED_64(dct_matrix_c);
    ASSERT_ALIGNED_64(dct_matrix_c_T);

    // float8 dct_T_regs[8];
    // float8 dct_C_regs[8];

    // // Pre-loading
    // #pragma UNROLL(8)
    // for(i = 0; i < 8; i++) {
    //     dct_T_regs[i] = *((float8 *)&dct_matrix_c_T[i * 8]);
    //     dct_C_regs[i] = *((float8 *)&dct_matrix_c[i * 8]);
    // }

    // #pragma MUST_ITERATE(2, , 2)
    for(b = 0; b < num_blocks; b++) {
        float8 intermediate_regs[8];
        float8* current_output = (float8*)(dct_coeffs + b * 64);
        int8_t* current_input_block = b_start + b * 64;
        
        #pragma MUST_ITERATE(8, 8, 8)
        // #pragma UNROLL(2)
        for(i = 0; i < 8; i++) {
            char8 in_char = *((char8 *)&current_input_block[i * 8]);
            float8 in_vec = __convert_float8(in_char);
    
            float8 row_acc = (float8)0.0f;                                                          // acumulator
    
            #pragma UNROLL(4) 
            for(k = 0; k < 8; k++) {
                float pixel_val = in_vec.s[k];   
                                                     
                // float8 c_row = dct_T_regs[k];        
                float8 c_row = *(float8*)(dct_matrix_c_T + k * 8);                                 // for each pixel in a row we calculate it's impact on the output row
                
                row_acc += c_row * pixel_val;
            }
    
            intermediate_regs[i] = row_acc;
        }
        
        #pragma MUST_ITERATE(8, 8, 8)
        // #pragma UNROLL(2)
        for(i = 0; i < 8; i++) {
            // float8 c_factors = dct_C_regs[i];
            float8 c_factors = *(float8*)(dct_matrix_c + i * 8);
            
            float8 row_acc = (float8)0.0f;
    
            #pragma UNROLL(4)
            for(k = 0; k < 8; k++) {
                float c_val = c_factors.s[k];
                
                float8 m_row = intermediate_regs[k];
                
                row_acc += m_row * c_val;
            }
    
            current_output[i] = row_acc;
        }

    }

}
