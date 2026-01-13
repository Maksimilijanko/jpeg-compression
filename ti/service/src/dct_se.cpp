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
    se_tmplt.DIMFMT  = __SE_DIMFMT_3D;                          // The structure is 3D
    se_tmplt.ICNT0   = 8;                                       // Row width
    se_tmplt.ICNT1   = 8;                                       // Column heigh
    se_tmplt.ICNT2   = num_blocks;                              // How many blocks
    se_tmplt.DIM1    = 8;                                       // 
    se_tmplt.DIM2    = 64;                                      // stride
    se_tmplt.PROMOTE = __SE_PROMOTE_OFF;                        // 
    
    __SE0_OPEN(input_buffer, se_tmplt);                         // Open the SE0 streaming engine interface

    // --- Streaming Address Generator setup ---
    __SA_TEMPLATE_v1 sa_tmplt = __gen_SA_TEMPLATE_v1();
    sa_tmplt.VECLEN  = __SA_VECLEN_8ELEMS;                      // 8 element vector length
    sa_tmplt.DIMFMT  = __SA_DIMFMT_3D;                          // 3D structure
    sa_tmplt.ICNT0   = 8;                                       
    sa_tmplt.ICNT1   = 8;
    sa_tmplt.ICNT2   = num_blocks;
    sa_tmplt.DIM1    = 8; 
    sa_tmplt.DIM2    = 64; 
    
    __SA0_OPEN(sa_tmplt);
    float8 * out_stream = (float8 *)output_buffer;

    for(int b = 0; b < num_blocks; b++) {
        
        float8 rows[8]; // Intermediate

        #pragma MUST_ITERATE(8, 8, 8)
        #pragma UNROLL(1) 
        for(int i = 0; i < 8; i++) {
            float8 pixels = __convert_float8(__convert_short8(strm_eng<0, char8>::get_adv()));

            float8 acc = (float8)0.0f;
            acc += trans[0] * pixels.s[0];
            acc += trans[1] * pixels.s[1];
            acc += trans[2] * pixels.s[2];
            acc += trans[3] * pixels.s[3];
            acc += trans[4] * pixels.s[4];
            acc += trans[5] * pixels.s[5];
            acc += trans[6] * pixels.s[6];
            acc += trans[7] * pixels.s[7];

            rows[i] = acc;
        }

        #pragma MUST_ITERATE(8, 8, 8)
        #pragma UNROLL(1)
        for(int i = 0; i < 8; i++) {
            
            float8 c_vec = coefs[i];
            
            float8 final_acc = (float8)0.0f;
            final_acc += rows[0] * c_vec.s[0];
            final_acc += rows[1] * c_vec.s[1];
            final_acc += rows[2] * c_vec.s[2];
            final_acc += rows[3] * c_vec.s[3];
            final_acc += rows[4] * c_vec.s[4];
            final_acc += rows[5] * c_vec.s[5];
            final_acc += rows[6] * c_vec.s[6];
            final_acc += rows[7] * c_vec.s[7];

            *strm_agen<0, float8>::get_adv(out_stream) = final_acc;
        }

    }

    

    __SE0_CLOSE();
    __SA0_CLOSE();
}
