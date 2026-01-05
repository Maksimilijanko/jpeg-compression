#ifndef DCT_H
#define DCT_H

#include <stdint.h>

#define PI 3.14159265358979323846f

/* DCT function declarations */

/*
* Structure for handling bit-level writing.
*/
typedef struct {
    uint8_t *buffer;    // Buffer in which we write encoded coefficients
    uint32_t byte_pos;  // Current byte in the buffer
    uint32_t bit_pos;   // Current bit position in the current byte (0-7)
    uint8_t current;    // Current byte being constructed
} BitWriter;

/*
*  Structure representing a Huffman code.
*/
typedef struct {
    uint16_t code;      // Code itself
    uint8_t len;        // Length of the code in bits
} HuffmanCode;

/*
* Structure representing a Variable Length Integer.
* Used for encoding non-zero DCT coefficients.
*/
typedef struct {
    uint16_t bits;      // Bits representing the value
    uint8_t len;        // Length of the bits
} VLI;                  // VLI - Variable Length Integer


/* Huffman tables */
extern const HuffmanCode huff_dc_lum[16];   // DC table
extern const HuffmanCode huff_ac_lum[256];  // AC table

/* 
* Standard Luminance quantization table.
*/
extern const uint8_t std_lum_qt[64];

/*
 * Centers the grayscale values around zero.
 * DCT works with cosine waves oscillating around zero.
 * This function shifts the grayscale values by subtracting 128.
 * Performs in-place modification of the grayscale_values array.
 */
void center_around_zero(float* grayscale_values, uint32_t width, uint32_t height);

/*
    * Splits the input image into 8x8 blocks.
    * Pads the image with edge pixels if necessary (clamp).
    * Also outputs the number of blocks in width and height via out_blocks_w and out_blocks_h.
    * Blocks are stored in row-major order.
    * Return value is stored in out_blocks parameter which should be pre-allocated by the caller.
    */
void image_to_blocks(float *image, uint32_t width, uint32_t height, uint32_t *out_blocks_w, uint32_t *out_blocks_h, float *out_blocks);

/*
    * Performs DCT on all 8x8 blocks.
    * Input: pointer to an array of blocks as returned by image_to_blocks
    * Input: number of blocks in width and height, 
    * Input: pointer to store the output DCT blocks.
    * Return value is stored in out_dct_blocks parameter which should be pre-allocated by the caller.
    */
void perform_dct(float *blocks, uint32_t blocks_w, uint32_t blocks_h, float *out_dct_blocks);

/*
    * Performs DCT on a single 8x8 block.
    * Input: pointer to an array of 64 floats (8x8 block).
    * Input: pointer to an array to store the DCT coefficients.
    * Return value is stored in out_dct_block parameter which should be pre-allocated by the caller.
    */
void perform_dct_one_block(float *block, float *out_dct_block);

/*
    * Quantizes a DCT block.
    * Input: pointer to an array of DCT coefficients for a single block.
    * Input: pointer to a quantization table.
    * Input: pointer to an array to store the quantized coefficients.
    * Each coefficient is represented as int16_t, as Baseline JPEG standard requires.
    * Return value is stored in out_quantized_block parameter which should be pre-allocated by the caller.
    */
void quantize_block(float *dct_block, int16_t* out_quantized_block);

/*
    * Encodes the DCT coefficients.
    * Input: pointer to an array of DCT coefficients for a single block. Coefficients need to be in zigzag order.
    * Input: pointer to an array to store the encoded data.
    * Input: pointer to a BitWriter (bit conitnuity is improtant for JFIF serialization)
    * Return value is stored in out_encoded_data parameter which should be pre-allocated by the caller.
    * Returns the actual DC coefficient, so it can be used as 'prev_dc' for the next block.
    * Also outputs the size of the encoded data via out_data_size parameter.
    */
int16_t encode_coefficients(int16_t *dct_block, int16_t prev_dc, uint8_t* out_encoded_data, BitWriter *bw);

/*
    * Serializes the encoded data to a file.
    * Input: filename to write to.
    * Input: pointer to an array of encoded coefficients.
    * Input: size of the encoded data.
    */
void serialize_to_file(const char *filename, uint8_t *encoded_data, uint32_t data_size);

/*
    * Reorders the quantized DCT coefficients in zigzag order.
    * Input: pointer to an array of 64 int16_t (quantized DCT coefficients).
    * Input: pointer to an array to store the reordered coefficients.
    * Return value is stored in out_block parameter which should be pre-allocated by the caller.
    */
void zigzag_order(const int16_t *input_block, int16_t *output_block);

/*
    * Writes a single byte to the BitWriter.
    * It ensures byte stuffing is performed so JFIF decoder can distinguish markers from encoded values.
    */
void bw_put_byte(BitWriter *bw, uint8_t val);

/*
    * Writes a code to the BitWriter.
    * Input: pointer to a BitWriter struct.
    * Input: code to write.
    * Input: length of the code in bits.
    */
void bw_write(BitWriter *bw, uint32_t code, int length);

/*
    * Returns the VLI (Variable Length Integer) representation of a value.
    * Input: value to encode.
    * Return: VLI structure containing the code and its length.
    */
VLI get_vli(int16_t value);


#endif
