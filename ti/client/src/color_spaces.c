#include "color_spaces.h"
#include <stdlib.h>

RGB* read_pixels(uint8_t* pixel_data, uint32_t width, uint32_t height, int orientation) {
    RGB* pixels = (RGB*)malloc(width * height * sizeof(RGB));
    if (pixels == NULL) {
        return NULL; 
    }

    // We need to take in consideration row padding for BMP format
    // Each row is padded to be a multiple of 4 bytes
    uint32_t padding = (4 - ((width * 3) % 4)) % 4;
    uint32_t row_stride = (width * 3) + padding;            // This is the actual width of each row in BMP, expressed in bytes

    // BMP pixel data is stored in BGR format
    for(uint32_t i = 0; i < height; i++) {
        for(uint32_t j = 0; j < width; j++) {
            uint32_t pixel_idx = i * width + j;             // linear index
            uint32_t bmp_row = orientation ? i : height - 1 - i;
            uint32_t bmp_index = bmp_row * row_stride + j * 3;    // index in BMP data
            
            pixels[pixel_idx].b = pixel_data[bmp_index];     
            pixels[pixel_idx].g = pixel_data[bmp_index + 1]; 
            pixels[pixel_idx].r = pixel_data[bmp_index + 2]; 
        }
    }

    return pixels;
}
