#include "color_spaces.h"
#include <stdlib.h>

RGB* read_pixels(uint8_t* pixel_data, uint32_t width, uint32_t height) {
    RGB* pixels = (RGB*)malloc(width * height * sizeof(RGB));
    if (pixels == NULL) {
        return NULL; 
    }

    // BMP pixel data is stored in BGR format
    for (uint32_t i = 0; i < width * height; i++) {
        pixels[i].b = pixel_data[i * 3 + 0];
        pixels[i].g = pixel_data[i * 3 + 1];
        pixels[i].r = pixel_data[i * 3 + 2];
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