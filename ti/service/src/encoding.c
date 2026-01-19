#include "jpeg_compression.h"
#include <c7x.h>

void bw_put_byte(BitWriter *bw, uint8_t val)
{
    bw->buffer[bw->byte_pos++] = val;

    if (val == 0xFF)
        bw->buffer[bw->byte_pos++] = 0x00;
}

static inline int ctz_64(uint64_t x)
{
    if (x == 0)
        return 64;
    uint64_t reversed = (uint64_t)__bit_reverse((unsigned long)x);

    return (int)__leftmost_bit_detect_one((unsigned long)reversed);
}

void flush_bits(BitWriter *restrict bw)
{
    if (bw->bit_pos > 0)
    {
        uint8_t byte_val = (uint8_t)(bw->current >> 56);

        bw->buffer[bw->byte_pos++] = byte_val;

        if (byte_val == 0xFF)
        {
            bw->buffer[bw->byte_pos++] = 0x00;
        }

        bw->bit_pos = 0;
        bw->current = 0;
    }
}

static inline void bw_write(BitWriter *restrict bw, uint32_t code, int length)
{
    int shift = 64 - bw->bit_pos - length;
    bw->current |= ((uint64_t)code << shift);
    bw->bit_pos += length;

    while (bw->bit_pos >= 8)
    {
        uint8_t byte_val = (uint8_t)(bw->current >> 56);

        bw->buffer[bw->byte_pos++] = byte_val;

        if (byte_val == 0xFF)
        {
            bw->buffer[bw->byte_pos++] = 0x00;
        }

        bw->current <<= 8;
        bw->bit_pos -= 8;
    }
}

int16_t encode_coefficients(int16_t *restrict dct_block, int16_t prev_dc, BitWriter *restrict bw)
{

    int16_t diff = dct_block[0] - prev_dc;

    int32_t abs_diff = __abs(diff);
    int len = 0;
    uint32_t bits = 0;

    if (abs_diff != 0)
    {
        len = 31 - __norm(abs_diff);
        int16_t temp = diff + (diff >> 15);
        bits = temp & ((1 << len) - 1);
    }

    HuffmanCode hc = huff_dc_lum[len];
    bw_write(bw, (hc.code << len) | bits, hc.len + len);

    short32 v_lo = *((short32 *)&dct_block[0]);
    short32 v_hi = *((short32 *)&dct_block[32]);

    __vpred pred_lo_zeros = __cmp_eq_pred(v_lo, (short32)0);
    __vpred pred_hi_zeros = __cmp_eq_pred(v_hi, (short32)0);

    uint64_t raw_lo = (uint64_t)__create_scalar(pred_lo_zeros);
    uint64_t raw_hi = (uint64_t)__create_scalar(pred_hi_zeros);

    uint64_t mask_const = 0x5555555555555555ULL;
    uint64_t nz_mask_lo = (~(raw_lo & (raw_lo >> 1) & mask_const)) & mask_const;
    uint64_t nz_mask_hi = (~(raw_hi & (raw_hi >> 1) & mask_const)) & mask_const;

    nz_mask_lo &= ~1ULL;

    int last_k = 0;

    const HuffmanCode *restrict ac_table = huff_ac_lum;
    const HuffmanCode huff_ZRL = ac_table[0xF0];

    while (nz_mask_lo != 0)
    {
        int bit_idx = ctz_64(nz_mask_lo);
        nz_mask_lo &= (nz_mask_lo - 1);
        int curr_k = bit_idx >> 1;

        int16_t val = dct_block[curr_k];
        int zero_run = curr_k - last_k - 1;

        if (zero_run >= 16)
        {
            int num_zrl = zero_run >> 4;
            zero_run = zero_run & 0xF;

            int z;
            for (z = 0; z < num_zrl; z++)
            {
                bw_write(bw, huff_ZRL.code, huff_ZRL.len);
            }
        }

        int32_t abs_val = __abs(val);
        int vli_len = 31 - __norm(abs_val);
        int16_t temp = val + (val >> 15);
        uint32_t vli_bits = temp & ((1 << vli_len) - 1);

        uint8_t symbol = (zero_run << 4) | vli_len;
        HuffmanCode ac_hc = ac_table[symbol];

        bw_write(bw, (ac_hc.code << vli_len) | vli_bits, ac_hc.len + vli_len);

        last_k = curr_k;
    }

    while (nz_mask_hi != 0)
    {
        int bit_idx = ctz_64(nz_mask_hi);
        nz_mask_hi &= (nz_mask_hi - 1);
        int curr_k = 32 + (bit_idx >> 1);

        int16_t val = dct_block[curr_k];
        int zero_run = curr_k - last_k - 1;

        if (zero_run >= 16)
        {
            int num_zrl = zero_run >> 4;
            int z = 0;
            zero_run = zero_run & 0xF;
            for (z = 0; z < num_zrl; z++)
            {
                bw_write(bw, huff_ZRL.code, huff_ZRL.len);
            }
        }

        int32_t abs_val = __abs(val);
        int vli_len = 31 - __norm(abs_val);
        int16_t temp = val + (val >> 15);
        uint32_t vli_bits = temp & ((1 << vli_len) - 1);

        uint8_t symbol = (zero_run << 4) | vli_len;
        HuffmanCode ac_hc = ac_table[symbol];

        bw_write(bw, (ac_hc.code << vli_len) | vli_bits, ac_hc.len + vli_len);

        last_k = curr_k;
    }

    if (last_k < 63)
    {
        bw_write(bw, ac_table[0x00].code, ac_table[0x00].len);
    }

    return dct_block[0];
}
