#include "jpeg_compression.h"
#include <c7x.h>

#pragma DATA_ALIGN(perm_mask_lo, 64)
static uchar64 perm_mask_lo;

#pragma DATA_ALIGN(perm_mask_hi, 64)
static uchar64 perm_mask_hi;

const static uint8_t zigzag_map[64] = {
     0,  1,  8, 16,  9,  2,  3, 10,
    17, 24, 32, 25, 18, 11,  4,  5,
    12, 19, 26, 33, 40, 48, 41, 34,
    27, 20, 13,  6,  7, 14, 21, 28,
    35, 42, 49, 56, 57, 50, 43, 36,
    29, 22, 15, 23, 30, 37, 44, 51,
    58, 59, 52, 45, 38, 31, 39, 46,
    53, 60, 61, 54, 47, 55, 62, 63
};

void init_zigzag(void) {
    uint8_t temp_lo[64];
    uint8_t temp_hi[64];
    int i;

    for (i = 0; i < 32; i++) {
        uint8_t src_idx = zigzag_map[i];
        temp_lo[2 * i]     = (uint8_t)(src_idx * 2);    
        temp_lo[2 * i + 1] = (uint8_t)(src_idx * 2 + 1); 
    }

    for (i = 32; i < 64; i++) {
        uint8_t src_idx = zigzag_map[i];
        int j = i - 32;
        temp_hi[2 * j]     = (uint8_t)(src_idx * 2);
        temp_hi[2 * j + 1] = (uint8_t)(src_idx * 2 + 1);
    }

    perm_mask_lo = *((uchar64 *)temp_lo);
    perm_mask_hi = *((uchar64 *)temp_hi);
}

static inline uchar64 vperm_byte_wrapper(uchar64 mask, uchar64 src_lo, uchar64 src_hi) {
    // Input data is short32. We can't operate directly on shorts, so we perform permutation on byte level.
    // We use extended zig-zag table which addresses each byte of input array and permutates it.
    // Because this extended zig-zag table has length of 128, we need to perform this permutation twice, once for each half of zigzag table.

    uchar64 p1 = __vperm_vvv(mask, src_lo);
    uchar64 p2 = __vperm_vvv(mask, src_hi);

    // p2 has the upper addresses.
    // p1 has the lower addresses.
    // __vperm_vvv reads addreses by modulo of 64, so it never "leaves" the input array.
    // We need to determine which array holds the right value for each index.
    // For indexes lower than 64, p1 has the correct value, and vice versa. 
    __vpred pred = __cmp_gt_pred(mask, (uchar64)63);

    return __select(pred, p2, p1);
}

void zigzag_order(const int16_t* restrict input_block, int16_t* restrict output_block, uint8_t num_blocks) {
    uint32_t b;

    const short32 *input_vec = (const short32 *)input_block;
    short32 *output_vec      = (short32 *)output_block;

    for(b = 0; b < num_blocks; b++) {
        
        short32 v_in0_s = input_vec[b * 2 + 0];
        short32 v_in1_s = input_vec[b * 2 + 1];

        uchar64 v_in0_u = as_uchar64(v_in0_s);
        uchar64 v_in1_u = as_uchar64(v_in1_s);

        // perform two permutations, once for each half of the zigzag table
        uchar64 v_out0_u = vperm_byte_wrapper(perm_mask_lo, v_in0_u, v_in1_u);
        uchar64 v_out1_u = vperm_byte_wrapper(perm_mask_hi, v_in0_u, v_in1_u);

        // Join partial results into once succesive array in memory
        output_vec[b * 2 + 0] = as_short32(v_out0_u);
        output_vec[b * 2 + 1] = as_short32(v_out1_u);
    }
}
