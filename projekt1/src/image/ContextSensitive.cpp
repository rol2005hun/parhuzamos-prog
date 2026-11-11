#include "ContextSensitive.h"
#include <omp.h>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <vector>

namespace ContextSensitive {

// Generate 1D Gauss kernel
std::vector<float> makeGaussKernel1D(int size) {
    // round size up to odd
    if (size % 2 == 0) ++size;
    size = std::clamp(size, 3, 9);

    std::vector<float> k(size);
    float sigma = size / 6.0f;
    float sum   = 0.0f;
    int   half  = size / 2;

    for (int i = 0; i < size; ++i) {
        float x = static_cast<float>(i - half);
        k[i] = std::exp(-(x * x) / (2.0f * sigma * sigma));
        sum += k[i];
    }
    for (auto& v : k) v /= sum;
    return k;
}

// Gauss blur - separable pass: horizontal, then vertical
Image gaussBlur(const Image& src, int kernelSize) {
    if (kernelSize % 2 == 0) ++kernelSize;
    kernelSize = std::clamp(kernelSize, 3, 9);

    auto kernel = makeGaussKernel1D(kernelSize);
    int  half   = kernelSize / 2;
    int  W      = src.width;
    int  H      = src.height;

    Image temp = src.clone(); // horizontal pass result
    Image out  = src.clone(); // vertical pass result

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float r = 0, g = 0, b = 0;
            for (int k = 0; k < kernelSize; ++k) {
                int sx = std::clamp(x + k - half, 0, W - 1);
                const float* p = src.at(sx, y);
                r += p[0] * kernel[k];
                g += p[1] * kernel[k];
                b += p[2] * kernel[k];
            }
            float* t = temp.at(x, y);
            t[0] = r; t[1] = g; t[2] = b;
        }
    }

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float r = 0, g = 0, b = 0;
            for (int k = 0; k < kernelSize; ++k) {
                int sy = std::clamp(y + k - half, 0, H - 1);
                const float* p = temp.at(x, sy);
                r += p[0] * kernel[k];
                g += p[1] * kernel[k];
                b += p[2] * kernel[k];
            }
            float* o = out.at(x, y);
            o[0] = r; o[1] = g; o[2] = b;
            // alpha remains unchanged
        }
    }

    return out;
}

// Edge detection - Sobel operator
// Gradient magnitude = sqrt(Gx^2 + Gy^2)
Image edgeDetect(const Image& src) {
    int W = src.width;
    int H = src.height;
    Image out = src.clone();

    // Sobel kernels
    // Gx: [-1, 0, 1; -2, 0, 2; -1, 0, 1]
    // Gy: [-1,-2,-1;  0, 0, 0;  1, 2, 1]
    static const int kx[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    static const int ky[3][3] = {{-1,-2,-1}, { 0, 0, 0}, { 1, 2, 1}};

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float gxr = 0, gyr = 0;
            float gxg = 0, gyg = 0;
            float gxb = 0, gyb = 0;
            for (int ky_i = 0; ky_i < 3; ++ky_i) {
                for (int kx_i = 0; kx_i < 3; ++kx_i) {
                    int sx = std::clamp(x + kx_i - 1, 0, W - 1);
                    int sy = std::clamp(y + ky_i - 1, 0, H - 1);
                    const float* p = src.at(sx, sy);
                    gxr += p[0] * kx[ky_i][kx_i];
                    gxg += p[1] * kx[ky_i][kx_i];
                    gxb += p[2] * kx[ky_i][kx_i];
                    gyr += p[0] * ky[ky_i][kx_i];
                    gyg += p[1] * ky[ky_i][kx_i];
                    gyb += p[2] * ky[ky_i][kx_i];
                }
            }
            float* o = out.at(x, y);
            o[0] = std::clamp(std::sqrt(gxr*gxr + gyr*gyr), 0.0f, 1.0f);
            o[1] = std::clamp(std::sqrt(gxg*gxg + gyg*gyg), 0.0f, 1.0f);
            o[2] = std::clamp(std::sqrt(gxb*gxb + gyb*gyb), 0.0f, 1.0f);
        }
    }
    return out;
}

// Unsharp masking
// out = src + amount * (src - blur(src))
Image unsharpMask(const Image& src, int kernelSize, float amount) {
    Image blurred = gaussBlur(src, kernelSize);
    Image out     = src.clone();
    int   px      = src.pixelCount();

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < px; ++i) {
        const float* s = src.pixels.data()     + i * 4;
        const float* b = blurred.pixels.data() + i * 4;
        float*       o = out.pixels.data()     + i * 4;
        for (int c = 0; c < 3; ++c) {
            o[c] = std::clamp(s[c] + amount * (s[c] - b[c]), 0.0f, 1.0f);
        }
        // alpha left unchanged
    }
    return out;
}

// Median filter - sorts elements of the kernel per channel
Image medianFilter(const Image& src, int kernelSize) {
    if (kernelSize % 2 == 0) ++kernelSize;
    kernelSize = std::clamp(kernelSize, 3, 9);

    int  half = kernelSize / 2;
    int  W    = src.width;
    int  H    = src.height;
    Image out = src.clone();

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < H; ++y) {
        std::vector<float> bufR, bufG, bufB;
        bufR.reserve(kernelSize * kernelSize);
        bufG.reserve(kernelSize * kernelSize);
        bufB.reserve(kernelSize * kernelSize);

        for (int x = 0; x < W; ++x) {
            bufR.clear(); bufG.clear(); bufB.clear();

            for (int ky = -half; ky <= half; ++ky) {
                for (int kx = -half; kx <= half; ++kx) {
                    int sx = std::clamp(x + kx, 0, W - 1);
                    int sy = std::clamp(y + ky, 0, H - 1);
                    const float* p = src.at(sx, sy);
                    bufR.push_back(p[0]);
                    bufG.push_back(p[1]);
                    bufB.push_back(p[2]);
                }
            }

            size_t mid = bufR.size() / 2;
            std::nth_element(bufR.begin(), bufR.begin() + mid, bufR.end());
            std::nth_element(bufG.begin(), bufG.begin() + mid, bufG.end());
            std::nth_element(bufB.begin(), bufB.begin() + mid, bufB.end());

            float* o = out.at(x, y);
            o[0] = bufR[mid];
            o[1] = bufG[mid];
            o[2] = bufB[mid];
            // alpha left unchanged
        }
    }
    return out;
}

} // namespace ContextSensitive
