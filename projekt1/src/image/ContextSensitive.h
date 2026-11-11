#pragma once
#include "Image.h"

// Context-SENSITIVE filters - CPU reference implementation
// Using OpenMP parallelization
// The GPU version is handled by the ComputeShader class.

namespace ContextSensitive {

// Valid kernel sizes: 3, 5, 7, 9
// (if kernelSize is even, +1 is added)

// Gauss blur — separable 1D × 1D pass
Image gaussBlur(const Image& src, int kernelSize);

// Sobel edge detection - gradient magnitude
Image edgeDetect(const Image& src);

// Unsharp masking: out = src + amount * (src - blurred)
Image unsharpMask(const Image& src, int kernelSize, float amount = 1.5f);

// Median filter — pixel sorting within the kernel
Image medianFilter(const Image& src, int kernelSize);

std::vector<float> makeGaussKernel1D(int size);

} // namespace ContextSensitive
