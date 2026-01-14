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

// Moramo imati globalni pointer samo za B kanal
// jer SE0 i SE1 brinu o R i G, ali za B moramo rucno pamtiti gde smo stali.
uchar32 * vec_b_ptr;

extern "C" void fetch_setup(uint8_t* r_vec, uint8_t* g_vec, uint8_t* b_vec, uint64_t image_length) {
    
    // --- Setup za R i G (SE0 i SE1) ---
    __SE_TEMPLATE_v1 se_params = __gen_SE_TEMPLATE_v1();

    se_params.ELETYPE = __SE_ELETYPE_8BIT;
    se_params.VECLEN  = __SE_VECLEN_32ELEMS; 
    se_params.ICNT0   = image_length; 
    se_params.DIM1    = 0;
    se_params.ICNT1   = 0;
    se_params.DIM2    = 0;
    se_params.ICNT2   = 0;

    // Otvaramo samo SE0 i SE1
    __SE0_OPEN((void*)r_vec, se_params);
    __SE1_OPEN((void*)g_vec, se_params);

    // --- Setup za B (Pointer) ---
    // Samo sacuvamo pocetnu adresu u globalnu promenljivu
    vec_b_ptr = (uchar32*) b_vec;
}

extern "C" void fetch_next_block(int8_t* y_output) {
    
    char32 * vec_y_out = (char32 *) y_output;

    short32 coeff_r = (short32) 77;
    short32 coeff_g = (short32) 150;
    short32 coeff_b = (short32) 29;
    short32 val_128 = (short32) 128;

    // -----------------------------------------------------------
    // PRVA POLOVINA BLOKA (0-31)
    // -----------------------------------------------------------
    
    // R i G dobijamo iz "magije" hardvera (SE)
    uchar32 r_in_1 = strm_eng<0, uchar32>::get_adv();
    uchar32 g_in_1 = strm_eng<1, uchar32>::get_adv();
    
    // B moramo rucno ucitati preko pointera
    uchar32 b_in_1 = vec_b_ptr[0];

    short32 r_s_1 = __convert_short32(r_in_1);
    short32 g_s_1 = __convert_short32(g_in_1);
    short32 b_s_1 = __convert_short32(b_in_1);

    short32 y_temp_1 = (r_s_1 * coeff_r) + (g_s_1 * coeff_g) + (b_s_1 * coeff_b);
    y_temp_1 = (y_temp_1 >> 8) - val_128;

    vec_y_out[0] = __convert_char32(y_temp_1);

    // -----------------------------------------------------------
    // DRUGA POLOVINA BLOKA (32-63)
    // -----------------------------------------------------------

    uchar32 r_in_2 = strm_eng<0, uchar32>::get_adv();
    uchar32 g_in_2 = strm_eng<1, uchar32>::get_adv();
    
    // Ucitavamo drugi deo B vektora (offset 1, jer je tip pointera uchar32)
    uchar32 b_in_2 = vec_b_ptr[1];

    short32 r_s_2 = __convert_short32(r_in_2);
    short32 g_s_2 = __convert_short32(g_in_2);
    short32 b_s_2 = __convert_short32(b_in_2);

    short32 y_temp_2 = (r_s_2 * coeff_r) + (g_s_2 * coeff_g) + (b_s_2 * coeff_b);
    y_temp_2 = (y_temp_2 >> 8) - val_128;

    vec_y_out[1] = __convert_char32(y_temp_2);

    // -----------------------------------------------------------
    // UPDATE POINTERA ZA B
    // -----------------------------------------------------------
    // SE0 i SE1 su se sami pomerili.
    // Mi moramo rucno pomeriti B pointer za 2 (2 * 32 bajta = 64 bajta = 1 blok)
    vec_b_ptr += 2;
}

extern "C" void fetch_close() {
    __SE0_CLOSE();
    __SE1_CLOSE();
}
