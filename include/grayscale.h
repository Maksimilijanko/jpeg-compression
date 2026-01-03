#ifndef GRAYSCALE_H
#define GRAYSCALE_H

#include <stdint.h>
#include "color_spaces.h"

/*
 * Grayscale conversion functions.
 */


/* * Converts an array of RGB pixels to grayscale.
 * Each pixel is represented by a single float value in grayscale - Y (luminance) component.
 */
float* convert_to_grayscale(RGB* pixels, uint32_t width, uint32_t height);

/*
 * Centers the grayscale values around zero.
 * DCT works with cosine waves oscillating around zero.
 * This function shifts the grayscale values by subtracting 128.
 */
void center_around_zero(float* grayscale_values, uint32_t width, uint32_t height);

#endif
