#ifndef DCT_H
#define DCT_H

#include <stdint.h>

#define PI 3.14159265358979323846f

/* DCT function declarations */

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
void quantize_block(float *dct_block, const uint8_t *quant_table, int16_t* out_quantized_block);

/*
    * Encodes the DCT coefficients.
    * Input: pointer to an array of DCT coefficients for a single block.
    * Input: pointer to an array to store the encoded data.
    * Input: pointer to store the size of the encoded data.
    * Return value is stored in out_encoded_data parameter which should be pre-allocated by the caller.
    * Also outputs the size of the encoded data via out_data_size parameter.
    */
void encode_coefficients(int16_t *dct_block, uint8_t* out_encoded_data, uint32_t *out_data_size);

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

#endif
