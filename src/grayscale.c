#include "grayscale.h"
#include <stdlib.h>

float* convert_to_grayscale(RGB* pixels, uint32_t width, uint32_t height) {
    // each pixel is represented by a single float value in grayscale - Y (luminance) component
    float* grayscale_values = (float*)malloc(width * height * sizeof(float));
    if (grayscale_values == NULL) {
        return NULL; 
    }

    for (uint32_t i = 0; i < width * height; i++) {

        // Using standard luminance calculation - ITU-R BT.601
        grayscale_values[i] = 0.299f * pixels[i].r + 0.587f * pixels[i].g + 0.114f * pixels[i].b;
    }

    return grayscale_values;
}

void center_around_zero(float* grayscale_values, uint32_t width, uint32_t height) {
    for (uint32_t i = 0; i < width * height; i++) {
        grayscale_values[i] -= 128.0f; 
    }
}