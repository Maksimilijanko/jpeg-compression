#include "jpeg_compression.h"
#include <c7x.h>

void bw_put_byte(BitWriter *bw, uint8_t val)
{
    bw->buffer[bw->byte_pos++] = val;


    // byte stuffing so JPEG decoder can distinguish JPEG markers from values
    if (val == 0xFF)
        bw->buffer[bw->byte_pos++] = 0x00;
}

static inline int ctz_64(uint64_t x)
{
    if (x == 0)
        return 64;
    // get bitreversed 64bit value
    uint64_t reversed = (uint64_t)__bit_reverse((unsigned long)x);

    // detect first rightmost one - equal to counting zeros from right
    return (int)__leftmost_bit_detect_one((unsigned long)reversed);
}

void flush_bits(BitWriter *restrict bw)
{
    // bw_write writes every 8b in memory, so the remaining bits can only be in the first (MSB) byte
    if (bw->bit_pos > 0)
    {
        uint8_t byte_val = (uint8_t)(bw->current >> 56);            // shift for 56 places to get the highest byte

        bw->buffer[bw->byte_pos++] = byte_val;

        // byte stuffing
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
    // calculate how many bits we need to shift the code value in order to OR it with current
    int shift = 64 - bw->bit_pos - length;
    // shift the new code for calculated numbre of bits
    bw->current |= ((uint64_t)code << shift);
    // increment current bit_pos for length of the new data
    bw->bit_pos += length;

    // if there is anything to flush out to buffer
    while (bw->bit_pos >= 8)
    {
        // isolate the MSB (we write from left in the current 64b because it is easier to flush it to buffer)
        uint8_t byte_val = (uint8_t)(bw->current >> 56);

        // writeback
        bw->buffer[bw->byte_pos++] = byte_val;

        // byte stuffing
        if (byte_val == 0xFF)
        {
            bw->buffer[bw->byte_pos++] = 0x00;
        }

        // clear out the MSB from the current 64b
        bw->current <<= 8;
        // decrement the current bit position in 64b current
        bw->bit_pos -= 8;
    }
}

int16_t encode_coefficients(int16_t *restrict dct_block, int16_t prev_dc, BitWriter *restrict bw)
{
    // difference between current DC coeff and the previous one
    int16_t diff = dct_block[0] - prev_dc;

    // calculate the absolute value (using intrinsic function) and write it 
    int32_t abs_diff = __abs(diff);
    int len = 0;
    uint32_t bits = 0;

    if (abs_diff != 0)
    {
        // calculate the length of code
        len = 31 - __norm(abs_diff);
        // formulate diff in one's complement
        int16_t temp = diff + (diff >> 15);
        // write only len lower bits
        bits = temp & ((1 << len) - 1);
    }

    // get huffman code for the dc coefficient
    HuffmanCode hc = huff_dc_lum[len];                              // get huffman code for category (key or DC coeffs is length (category) only)
    bw_write(bw, (hc.code << len) | bits, hc.len + len);            // writeout category and the difference itself in one's complement

    // we can't fit the entire dct_block into one register so it is separated in two registers
    short32 v_lo = *((short32 *)&dct_block[0]);
    short32 v_hi = *((short32 *)&dct_block[32]);
    // this will store dct_block in little endian manner into V regisers

    // identify the zeros in registers and store the information in a predicate (two bits per short in __vpred)
    __vpred pred_lo_zeros = __cmp_eq_pred(v_lo, short32(0));
    __vpred pred_hi_zeros = __cmp_eq_pred(v_hi, short32(0));

    // we conver the vpred into uint64_t where each bit identifies zero values in the coresponding register
    // we want he uint64_t to contain one bit per short, not two bits (like vpred does)
    uint64_t raw_lo = (uint64_t)__create_scalar(pred_lo_zeros);
    uint64_t raw_hi = (uint64_t)__create_scalar(pred_hi_zeros);

    // in order to create a mask in which one bit controls one short, we do the following:
    // create a mask which has alternating ones and zeros in memory : 01010101...
    uint64_t mask_const = 0x5555555555555555ULL;
    // raw_lo & (raw_lo >> 1) is equal to one where there are two successive 1's. 
    // raw_lo & (raw_lo >> 1) & mask_const is equal to one where there are two successive 1's and are aligned with alternating mask 1
    // ~ inverts that - zero becomes one
    // (~(raw_hi & (raw_hi >> 1) & mask_const)) & mask_const - take the one's aligned with mask 1
    uint64_t nz_mask_lo = (~(raw_lo & (raw_lo >> 1) & mask_const)) & mask_const;
    uint64_t nz_mask_hi = (~(raw_hi & (raw_hi >> 1) & mask_const)) & mask_const;
    // the resulting masks contain single 1's for each zero short value

    nz_mask_lo &= ~1ULL;

    int last_k = 0;

    const HuffmanCode *restrict ac_table = huff_ac_lum;
    const HuffmanCode huff_ZRL = ac_table[0xF0];

    // iterate through first 32 shorts
    while (nz_mask_lo != 0)
    {
        int bit_idx = ctz_64(nz_mask_lo);               // find the first non-zero index from the right
        // to find the first dct element that is non-zero, we count zeros from the right because the dct coeffs are stored in LE manner into registers

        nz_mask_lo &= (nz_mask_lo - 1);                 // remove it (we are processing it now)
        int curr_k = bit_idx >> 1;                      // divide by two (because we are using two bits per short so the right index in terms of shorts is /2)

        // TODO: use vector scalar extraction
        // int16_t val = dct_block[curr_k];                // fetch that selected short value
        int16_t val = v_lo.s[curr_k];
        int zero_run = curr_k - last_k - 1;             // calculate the zero run 

        if (zero_run >= 16)                             // if run is longer than 16, write the ZRL symbols
        {
            int num_zrl = zero_run >> 4;                // divide by 16 to know how many ZRL's to write
            zero_run = zero_run & 0xF;                  // mod 16 (remainder)

            int z;
            for (z = 0; z < num_zrl; z++)
            {
                bw_write(bw, huff_ZRL.code, huff_ZRL.len);  // write ZRL codes
            }
        }

        // code non-zero value in one's complement
        int32_t abs_val = __abs(val);
        // count non-zero value length - category
        int vli_len = 31 - __norm(abs_val);
        int16_t temp = val + (val >> 15);
        uint32_t vli_bits = temp & ((1 << vli_len) - 1);            // code the value in one's complement

        uint8_t symbol = (zero_run << 4) | vli_len;             // crete (r, s) - r is run length, s is category and each takes 4 bits
        HuffmanCode ac_hc = ac_table[symbol];                   // code (r, s) in huffman code

        bw_write(bw, (ac_hc.code << vli_len) | vli_bits, ac_hc.len + vli_len);      // write out (r, s) huffman code and non-zero value in one's complement

        last_k = curr_k;            // set the value for next run
    }

    // repeat the same thing for the other half of the block
    while (nz_mask_hi != 0)
    {
        int bit_idx = ctz_64(nz_mask_hi);
        nz_mask_hi &= (nz_mask_hi - 1);
        int curr_k = 32 + (bit_idx >> 1);

        // int16_t val = dct_block[curr_k];
        int16_t val = v_hi.s[bit_idx >> 1];
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

    // if last non zero value was before the last element, write EOB code
    if (last_k < 63)
    {
        bw_write(bw, ac_table[0x00].code, ac_table[0x00].len);
    }

    return dct_block[0];
}
