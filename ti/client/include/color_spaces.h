#ifndef COLOR_SPACHES_H
#define COLOR_SPACHES_H

#include <stdint.h>

/*
 * Color space conversion functions and structures for representing colors.
 */

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} RGB;

typedef struct {
    uint8_t y;
    uint8_t cb;
    uint8_t cr;
} YCbCr;

/*
* Constructs RGB array from pixel data.
* orientation - defines reading orientation. Values:
*               1 - bottom-up
*               0 - top-down
* Return value is a dynamically allocated buffer - caller is responsible for its releasing.
*/
RGB* read_pixels(uint8_t* pixel_data, uint32_t width, uint32_t height, int orientation);


YCbCr* rgb_to_ycbcr(RGB* pixels, uint32_t width, uint32_t height);

#endif
