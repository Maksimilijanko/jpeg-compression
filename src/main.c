#include "dct.h"
#include "bmp_handler.h"
#include "grayscale.h"
#include "jfif_handler.h"
#include "color_spaces.h"
#include <stdlib.h>

int main(int argc, char **argv) {
    PARAMETERS params = parse_parameters(argc, argv);

    BMP_IMAGE image = load_bmp_image(params.inputFile); 

    // pixels is dyn. allocated - needs to be freed
    RGB* pixels = read_pixels(image.buffer, image.info.width, image.info.height, image.info.height <= 0);

    printf("BMP image imported.\n");

    // grayscale_y is dyn. allocated - needs to be freed
    float* grayscale_y = convert_to_grayscale(pixels, image.info.width, image.info.height);
    free(pixels);

    // In-place transformation
    center_around_zero(grayscale_y, image.info.width, image.info.height);

    printf("Pixels converted to Y and centered around zero.\n");

    uint32_t blocks_w;           
    uint32_t blocks_h;
    float* blocks;
    image_to_blocks(grayscale_y, image.info.width, image.info.height, &blocks_h, &blocks_w, &blocks);
    free(grayscale_y);

    printf("Image segmantation completed.\n");
    
    float *dct_coeffs = (float*)calloc(1, blocks_w * blocks_h * 64 * sizeof(float));
    perform_dct(blocks, blocks_w, blocks_h, dct_coeffs);
    free(blocks);

    printf("DCT completed.\n");

    uint32_t buffer_size = image.info.width * image.info.height * 2; 
    if (buffer_size < 4096) buffer_size = 4096; // Minimum 4KB

    uint8_t *encoded_buffer = (uint8_t*)malloc(buffer_size);
    
    BitWriter bw;
    bw.buffer = encoded_buffer;
    bw.byte_pos = 0;
    bw.bit_pos = 0;
    bw.current = 0;

    int16_t prev_dc = 0;
    uint32_t total_blocks = blocks_w * blocks_h;

    int16_t quantized_block[64];
    int16_t zigzag_block[64];

    for (uint32_t i = 0; i < total_blocks; i++) {
        float *current_dct_ptr = &dct_coeffs[i * 64];

        quantize_block(current_dct_ptr, quantized_block);

        zigzag_order(quantized_block, zigzag_block);

        prev_dc = encode_coefficients(zigzag_block, prev_dc, &bw);
    }

    if (bw.bit_pos > 0) {
        bw_put_byte(&bw, bw.current);
    }

    printf("Encoding completed.\n");
    printf("Original size (Raw Y): %u bytes\n", image.info.width * image.info.height);
    printf("Compressed size (Scan Data): %u bytes\n", bw.byte_pos);

    FILE *f_out = fopen(params.outputFile, "wb");
    if(f_out) {
        write_to_jfif(f_out, bw.buffer, bw.byte_pos, image.info.width, image.info.height);
        fclose(f_out);
        printf("JFIF serialization completed.\n");
    }

    free(dct_coeffs);
    free(encoded_buffer);
    free(image.buffer);

    return 0;
}
