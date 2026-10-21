#ifndef RS2_JPEG_H
#define RS2_JPEG_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Baseline JFIF decoder for title.dat. The original client decodes it with
// java.awt.Toolkit (IJG libjpeg 6b defaults: islow IDCT, fancy upsampling,
// table-driven YCbCr->RGB), so this follows the same integer arithmetic.
// On success *pixels is malloc'd ARGB (alpha 0xFF, as PixelGrabber returns).
bool jpeg_decode(const uint8_t *data, size_t length, int32_t *width, int32_t *height, uint32_t **pixels);

#endif // RS2_JPEG_H
