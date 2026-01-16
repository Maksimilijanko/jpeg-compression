#include "jpeg_compression.h"
#include <stdint.h>
#include <utils/mem/include/app_mem.h>
#include <math.h>
#include <c7x.h>
#include <c7x_scalable.h>

#include "jpeg_compression.h"
#include <stdint.h>
#include <c7x.h>
#include <c7x_scalable.h>

using namespace c7x;


extern "C" void fetch_setup(uint8_t* r_vec, uint8_t* gb_vec, uint64_t image_length) {
    
    __SE_TEMPLATE_v1 se_params = __gen_SE_TEMPLATE_v1();

    se_params.ELETYPE = __SE_ELETYPE_8BIT;
    se_params.VECLEN  = __SE_VECLEN_32ELEMS; 
    se_params.ICNT0   = image_length;               // we are fetching R from the frist SE
    se_params.DIM1    = 0;
    se_params.ICNT1   = 0;
    se_params.DIM2    = 0;
    se_params.ICNT2   = 0;

    
    __SE_TEMPLATE_v1 se_params2 = __gen_SE_TEMPLATE_v1();
    se_params2.ELETYPE = __SE_ELETYPE_8BIT;
    se_params2.VECLEN  = __SE_VECLEN_64ELEMS; 
    se_params2.ICNT0   = image_length * 2;           // we are fetching B and G from the frist SE
    se_params2.DIM1    = 0;
    se_params2.ICNT1   = 0;
    se_params2.DIM2    = 0;
    se_params2.ICNT2   = 0;


    __SE0_OPEN((void*)r_vec, se_params);
    __SE1_OPEN((void*)gb_vec, se_params2);               // the second one is going to be fetching twice as much

}

extern "C" void fetch_next_blocks(int8_t* y_output, uint16_t num_blocks) {
    
    char32 * vec_y_out = (char32 *) y_output;

    short32 coeff_r = (short32) 77;
    short32 coeff_g = (short32) 150;
    short32 coeff_b = (short32) 29;
    short32 val_128 = (short32) 128;

    uint16_t i = 0;
    for(i = 0; i < num_blocks; i++) {
        // Fetch the R components from the first one
        uchar32 r_in_1 = strm_eng<0, uchar32>::get_adv();
        uchar64 gb_in_1 = strm_eng<1, uchar64>::get_adv();
    
        // Upcasting
        short32 r_s_1 = __convert_short32(r_in_1);
        short32 g_s_1 = __convert_short32(gb_in_1.lo);
        short32 b_s_1 = __convert_short32(gb_in_1.hi);
    
        short32 y_temp_1 = (r_s_1 * coeff_r) + (g_s_1 * coeff_g) + (b_s_1 * coeff_b);
        y_temp_1 = (y_temp_1 >> 8) - val_128;
    
        vec_y_out[2*i + 0] = __convert_char32(y_temp_1);
    
        // We repeat everything, but for the second half of the block
    
        uchar32 r_in_2 = strm_eng<0, uchar32>::get_adv();
        uchar64 gb_in_2 = strm_eng<1, uchar64>::get_adv();
    
        short32 r_s_2 = __convert_short32(r_in_2);
        short32 g_s_2 = __convert_short32(gb_in_2.lo);
        short32 b_s_2 = __convert_short32(gb_in_2.hi);
    
        short32 y_temp_2 = (r_s_2 * coeff_r) + (g_s_2 * coeff_g) + (b_s_2 * coeff_b);
        y_temp_2 = (y_temp_2 >> 8) - val_128;
    
        vec_y_out[2*i + 1] = __convert_char32(y_temp_2);

    }

}

extern "C" void fetch_close() {
    __SE0_CLOSE();
    __SE1_CLOSE();
}
