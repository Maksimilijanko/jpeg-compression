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
            
            pixels[pixel_idx].b = (float)pixel_data[bmp_index];     
            pixels[pixel_idx].g = (float)pixel_data[bmp_index + 1]; 
            pixels[pixel_idx].r = (float)pixel_data[bmp_index + 2]; 
        }
    }

    return pixels;
}

YCbCr* rgb_to_ycbcr(RGB* pixels, uint32_t width, uint32_t height) {
    YCbCr* ycbcr_pixels = (YCbCr*)malloc(width * height * sizeof(YCbCr));
    if (ycbcr_pixels == NULL) {
        return NULL; 
    }

    for (uint32_t i = 0; i < width * height; i++) {
        float r = pixels[i].r;
        float g = pixels[i].g;
        float b = pixels[i].b;

        // Conversion formulas from RGB to YCbCr
        // Using ITU-R BT.601 standard
        ycbcr_pixels[i].y  =  0.299f * r + 0.587f * g + 0.114f * b;
        ycbcr_pixels[i].cb = -0.168736f * r - 0.331264f * g + 0.5f * b + 128.0f;
        ycbcr_pixels[i].cr =  0.5f * r - 0.418688f * g - 0.081312f * b + 128.0f;
    }

    return ycbcr_pixels;
}
