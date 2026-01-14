#include "jpeg_compression.h"
#include <stdint.h>
#include <utils/mem/include/app_mem.h>
#include <math.h>
#include <c7x.h>
#include <c7x_scalable.h>

using namespace c7x;

uchar32 * vec_r_src;
uchar32 * vec_g_src;
uchar32 * vec_b_src;

extern "C" void fetch_setup(uint8_t* r_vec, uint8_t* g_vec, uint8_t* b_vec) {
    vec_r_src = (uchar32*) r_vec;
    vec_g_src = (uchar32*) g_vec;
    vec_b_src = (uchar32*) b_vec;
}

extern "C" void fetch_next_block(int8_t* y_output) {
    
    char32 * vec_y_out = (char32 *) y_output;

    short32 coeff_r = (short32) 77;
    short32 coeff_g = (short32) 150;
    short32 coeff_b = (short32) 29;
    short32 val_128 = (short32) 128;

    // Upper half of one block. We load it and upcast it to 512b (32 shorts).
    uchar32 r_in_1 = vec_r_src[0]; 
    uchar32 g_in_1 = vec_g_src[0];
    uchar32 b_in_1 = vec_b_src[0];

    // Upcasting uchar32 to short32 to take up one V register in whole. We need this upcast because of Y conversion and multiplying by short.
    short32 r_s_1 = __convert_short32(r_in_1);
    short32 g_s_1 = __convert_short32(g_in_1);
    short32 b_s_1 = __convert_short32(b_in_1);

    // Perform SIMD instructions
    short32 y_temp_1 = (r_s_1 * coeff_r) + (g_s_1 * coeff_g) + (b_s_1 * coeff_b);

    y_temp_1 = (y_temp_1 >> 8) - val_128;

    // We write out the result, but downcast it to char32 first.
    vec_y_out[0] = __convert_char32(y_temp_1);

    // We repeat this for the lower half of the block.
    uchar32 r_in_2 = vec_r_src[1]; 
    uchar32 g_in_2 = vec_g_src[1];
    uchar32 b_in_2 = vec_b_src[1];

    // Upcast  
    short32 r_s_2 = __convert_short32(r_in_2);
    short32 g_s_2 = __convert_short32(g_in_2);
    short32 b_s_2 = __convert_short32(b_in_2);

    short32 y_temp_2 = (r_s_2 * coeff_r) + (g_s_2 * coeff_g) + (b_s_2 * coeff_b);

    y_temp_2 = (y_temp_2 >> 8) - val_128;

    vec_y_out[1] = __convert_char32(y_temp_2);

    // We have laoded two char32 vectors while loading a single block, so we increment pointers by 2
    vec_r_src += 2;
    vec_g_src += 2;
    vec_b_src += 2;
}
