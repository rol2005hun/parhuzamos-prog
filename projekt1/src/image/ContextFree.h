#pragma once
#include "Image.h"

// Context-FREE filters
// Implementation: AVX2 SIMD intrinsics + OpenMP parallelization
//
// Every filter modifies the image in-place, or returns a new Image.
// The "_simd" suffix indicates the AVX2 path.

namespace ContextFree {

// Brightness: adds a value to all channels [-1, +1]
void brightness(Image& img, float value);

// Exposure: multiplies pixel values (0.1 - 5.0)
void exposure(Image& img, float value);

// Contrast: increase/decrease contrast (0.0 - 3.0, 1.0 = no change)
void contrast(Image& img, float value);

// Gamma: power function correction (0.1 - 5.0, 1.0 = no change)
void gamma(Image& img, float value);

// Hue shift: rotates hue in HSV space [-180°, +180°]
void hueShift(Image& img, float degrees);

// Vignette: darkens corners (0.0 - 1.0)
void vignette(Image& img, float strength);


// Brightness for 8 pixels at once (32 floats = 256 bit)
void brightness_avx2(float* pixels, int count, float value);

// Exposure for 8 pixels at once
void exposure_avx2(float* pixels, int count, float value);

// Contrast factor computation and application
void contrast_avx2(float* pixels, int count, float factor);

} // namespace ContextFree
