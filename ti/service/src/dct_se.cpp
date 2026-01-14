#include "jpeg_compression.h"
#include <stdint.h>
#include <utils/mem/include/app_mem.h>
#include <math.h>
#include <c7x.h>
#include <c7x_scalable.h>

using namespace c7x;

extern const float dct_matrix_c[64];
extern const float dct_matrix_c_T[64];

static inline float8 load_unaligned_row(const float * ptr) {
    float8 v = (float8)0.0f;
    v.s[0] = ptr[0];
    v.s[1] = ptr[1];
    v.s[2] = ptr[2];
    v.s[3] = ptr[3];
    v.s[4] = ptr[4];
    v.s[5] = ptr[5];
    v.s[6] = ptr[6];
    v.s[7] = ptr[7];
    return v;
}

extern "C" void perform_dct_on_image(int8_t * __restrict__ input_buffer, 
                                     float * __restrict__ output_buffer, 
                                     int num_blocks) 
{
    float8 trans[8];
    float8 coefs[8];

    #pragma UNROLL(8) 
    for(int i=0; i<8; i++) {
        trans[i] = load_unaligned_row(&dct_matrix_c_T[i * 8]);
        coefs[i] = load_unaligned_row(&dct_matrix_c[i * 8]);
    }

    // --- Streaming Engine Setup ---
    __SE_TEMPLATE_v1 se_tmplt = __gen_SE_TEMPLATE_v1();
    se_tmplt.ELETYPE = __SE_ELETYPE_8BIT;                       // element type is 8bit long
    se_tmplt.VECLEN  = __SE_VECLEN_8ELEMS;                      // vector length is 8 elements (one row)
    se_tmplt.DIMFMT  = __SE_DIMFMT_2D;                          // The structure is 3D
    se_tmplt.ICNT0   = 64;                                       // Row width (in uints, not V types...)
    se_tmplt.ICNT1   = num_blocks * 64;                         // How many uints in total (not byte level, but DATA level (how many floats, chars, etc.))
    se_tmplt.DIM1    = 64;                                      // stride
    se_tmplt.PROMOTE = __SE_PROMOTE_OFF;                        // 
    
    __SE0_OPEN(input_buffer, se_tmplt);                         // Open the SE0 streaming engine interface

    // --- Streaming Address Generator setup ---
    __SA_TEMPLATE_v1 sa_tmplt = __gen_SA_TEMPLATE_v1();
    sa_tmplt.VECLEN  = __SA_VECLEN_8ELEMS;                      // 8 element vector length
    sa_tmplt.DIMFMT  = __SA_DIMFMT_2D;                          // 3D structure
    sa_tmplt.ICNT0   = 64;                                       // first dimension size IN FLOATS (not V Types, the SA unit will divide itself...)
    sa_tmplt.ICNT1   = num_blocks * 64;                         // how many floats in total (same as before with the SE...)
    sa_tmplt.DIM1    = 64;
    
    __SA0_OPEN(sa_tmplt);
    float8 * out_stream = (float8 *)output_buffer;

    for(int b = 0; b < num_blocks; b++) {
        
        float8 dct_rows[8]; // Intermediate
        float8 input_row;

        #pragma MUST_ITERATE(8, 8, 8)
        #pragma UNROLL(4) 
        for(int i = 0; i < 8; i++) {

            char8 raw = strm_eng<0, char8>::get_adv();
            input_row = __convert_float8(__convert_short8(raw));

            float8 acc = (float8)0.0f;
            #pragma MUST_ITERATE(8, 8, 8)
            #pragma UNROLL(8)
            for(int j = 0; j < 8; j++) {
                acc += trans[j] * input_row.s[j];
            }

            
            // acc += trans[0] * input_rows[i].s[0];
            // acc += trans[1] * input_rows[i].s[1];
            // acc += trans[2] * input_rows[i].s[2];
            // acc += trans[3] * input_rows[i].s[3];
            // acc += trans[4] * input_rows[i].s[4];
            // acc += trans[5] * input_rows[i].s[5];
            // acc += trans[6] * input_rows[i].s[6];
            // acc += trans[7] * input_rows[i].s[7];

            dct_rows[i] = acc;
        }

        #pragma MUST_ITERATE(8, 8, 8)
        #pragma UNROLL(4)
        for(int i = 0; i < 8; i++) {
            
            float8 c_vec = coefs[i];
            
            float8 final_acc = (float8)0.0f;
            #pragma MUST_ITERATE(8, 8, 8)
            #pragma UNROLL(8)
            for(int j = 0; j < 8; j++) {
                final_acc += dct_rows[j] * c_vec.s[j];
            }
            // final_acc += dct_rows[0] * c_vec.s[0];
            // final_acc += dct_rows[1] * c_vec.s[1];
            // final_acc += dct_rows[2] * c_vec.s[2];
            // final_acc += dct_rows[3] * c_vec.s[3];
            // final_acc += dct_rows[4] * c_vec.s[4];
            // final_acc += dct_rows[5] * c_vec.s[5];
            // final_acc += dct_rows[6] * c_vec.s[6];
            // final_acc += dct_rows[7] * c_vec.s[7];

            // // Grupa 1 (Parni) - Nadamo se da ce ovo kompajler staviti na jednu stranu
            // float8 p0 = dct_rows[0] * c_vec.s[0];
            // float8 p2 = dct_rows[2] * c_vec.s[2];
            // float8 p4 = dct_rows[4] * c_vec.s[4];
            // float8 p6 = dct_rows[6] * c_vec.s[6];
            // float8 sum_even = (p0 + p2) + (p4 + p6);

            // // Grupa 2 (Neparni) - Nadamo se da ce ovo staviti na drugu stranu
            // float8 p1 = dct_rows[1] * c_vec.s[1];
            // float8 p3 = dct_rows[3] * c_vec.s[3];
            // float8 p5 = dct_rows[5] * c_vec.s[5];
            // float8 p7 = dct_rows[7] * c_vec.s[7];
            // float8 sum_odd = (p1 + p3) + (p5 + p7);

            // // 4. KORAK: Finalni zbir (Nivo 3)
            // float8 final_acc = sum_even + sum_odd;


            // *(out_stream + b*8 + i) = final_acc;                     // DEBUG 
            *strm_agen<0, float8>::get_adv(out_stream) = final_acc;
        }

    
    }
    

    __SE0_CLOSE();
    __SA0_CLOSE();

}
