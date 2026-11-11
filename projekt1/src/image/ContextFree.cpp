#include "ContextFree.h"
#include <immintrin.h>   // AVX2 intrinsics
#include <omp.h>         // OpenMP
#include <cmath>
#include <algorithm>

namespace ContextFree {

// AVX2 segédfüggvények

// Hozzáad egy skalárt 'count' float elemhez, AVX2-vel (8 float / iteráció)
void brightness_avx2(float* pixels, int count, float value) {
    __m256 vVal = _mm256_set1_ps(value);
    int i = 0;
    // Fő hurok: 8 float egyszerre
    for (; i <= count - 8; i += 8) {
        __m256 v = _mm256_loadu_ps(pixels + i);
        v = _mm256_add_ps(v, vVal);
        _mm256_storeu_ps(pixels + i, v);
    }
    // Maradék elemek skalárisan
    for (; i < count; ++i) pixels[i] += value;
}

// Megszoroz 'count' float elemet egy skalárral, AVX2-vel
void exposure_avx2(float* pixels, int count, float value) {
    __m256 vVal = _mm256_set1_ps(value);
    int i = 0;
    for (; i <= count - 8; i += 8) {
        __m256 v = _mm256_loadu_ps(pixels + i);
        v = _mm256_mul_ps(v, vVal);
        _mm256_storeu_ps(pixels + i, v);
    }
    for (; i < count; ++i) pixels[i] *= value;
}

// Kontrasztkorrekció: formula: out = (in - 0.5) * factor + 0.5
// factor = 259*(c+255) / (255*(259-c)), ahol c a kontrasz [-255, 255] tartományban
void contrast_avx2(float* pixels, int count, float factor) {
    __m256 vFactor  = _mm256_set1_ps(factor);
    __m256 vHalf    = _mm256_set1_ps(0.5f);
    int i = 0;
    for (; i <= count - 8; i += 8) {
        __m256 v = _mm256_loadu_ps(pixels + i);
        // (v - 0.5) * factor + 0.5
        v = _mm256_sub_ps(v, vHalf);
        v = _mm256_mul_ps(v, vFactor);
        v = _mm256_add_ps(v, vHalf);
        _mm256_storeu_ps(pixels + i, v);
    }
    for (; i < count; ++i)
        pixels[i] = (pixels[i] - 0.5f) * factor + 0.5f;
}

// Brightness
void brightness(Image& img, float value) {
    float* data = img.pixels.data();
    int total   = img.pixelCount() * 4; // RGBA, de alpha-t is toljuk

    // OpenMP: a teljes tömböt részekre osztjuk szálak között
    #pragma omp parallel for schedule(static)
    for (int chunk = 0; chunk < omp_get_max_threads(); ++chunk) {
        int start = chunk * (total / omp_get_max_threads());
        int end   = (chunk == omp_get_max_threads() - 1)
                    ? total
                    : start + total / omp_get_max_threads();
        brightness_avx2(data + start, end - start, value);
    }

    // Clip [0, 1] és Alpha integritás megőrzése (ne váljon áttetszővé a kép)
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < img.pixelCount(); ++i) {
        data[i * 4 + 0] = std::clamp(data[i * 4 + 0], 0.0f, 1.0f);
        data[i * 4 + 1] = std::clamp(data[i * 4 + 1], 0.0f, 1.0f);
        data[i * 4 + 2] = std::clamp(data[i * 4 + 2], 0.0f, 1.0f);
        data[i * 4 + 3] = 1.0f;
    }
}

// Exposure
void exposure(Image& img, float value) {
    float* data = img.pixels.data();
    int total   = img.pixelCount() * 4;

    #pragma omp parallel for schedule(static)
    for (int chunk = 0; chunk < omp_get_max_threads(); ++chunk) {
        int start = chunk * (total / omp_get_max_threads());
        int end   = (chunk == omp_get_max_threads() - 1)
                    ? total
                    : start + total / omp_get_max_threads();
        exposure_avx2(data + start, end - start, value);
    }

    // Clip [0, 1] és Alpha integritás megőrzése
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < img.pixelCount(); ++i) {
        data[i * 4 + 0] = std::clamp(data[i * 4 + 0], 0.0f, 1.0f);
        data[i * 4 + 1] = std::clamp(data[i * 4 + 1], 0.0f, 1.0f);
        data[i * 4 + 2] = std::clamp(data[i * 4 + 2], 0.0f, 1.0f);
        data[i * 4 + 3] = 1.0f;
    }
}

// Contrast
// Képlet: factor = (259 * (c + 255)) / (255 * (259 - c))
// ahol c a csúszka értéke -255..255 tartományban (mi -1..1 skálán kapjuk)
void contrast(Image& img, float value) {
    // value: 0.0 = nincs változás, ±1.0 = max effect
    // Konvertálás a képlethez: c = value * 255
    float c      = value * 255.0f;
    float factor = (259.0f * (c + 255.0f)) / (255.0f * (259.0f - c));

    float* data = img.pixels.data();
    int total   = img.pixelCount() * 4;

    #pragma omp parallel for schedule(static)
    for (int chunk = 0; chunk < omp_get_max_threads(); ++chunk) {
        int start = chunk * (total / omp_get_max_threads());
        int end   = (chunk == omp_get_max_threads() - 1)
                    ? total
                    : start + total / omp_get_max_threads();
        contrast_avx2(data + start, end - start, factor);
    }

    // Clip [0, 1] és Alpha integritás megőrzése
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < img.pixelCount(); ++i) {
        data[i * 4 + 0] = std::clamp(data[i * 4 + 0], 0.0f, 1.0f);
        data[i * 4 + 1] = std::clamp(data[i * 4 + 1], 0.0f, 1.0f);
        data[i * 4 + 2] = std::clamp(data[i * 4 + 2], 0.0f, 1.0f);
        data[i * 4 + 3] = 1.0f;
    }
}

// Gamma
// out = in ^ gamma  (csak RGB-re, alpha-t meghagyjuk)
void gamma(Image& img, float value) {
    if (value <= 0.0f) value = 0.001f;
    float* data = img.pixels.data();
    int px      = img.pixelCount();

    // Alpha csatorna (index 3, 7, 11, ...) meghagyása → pixelenként járunk
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < px; ++i) {
        float* p = data + i * 4;
        p[0] = std::pow(std::max(p[0], 0.0f), value);
        p[1] = std::pow(std::max(p[1], 0.0f), value);
        p[2] = std::pow(std::max(p[2], 0.0f), value);
        // p[3] alpha — nem módosítjuk
    }
}

// Hue shift — RGB ↔ HSV konverzió
static void rgb2hsv(float r, float g, float b, float& h, float& s, float& v) {
    float mx = std::max({r, g, b});
    float mn = std::min({r, g, b});
    float d  = mx - mn;
    v = mx;
    s = (mx > 0.0f) ? d / mx : 0.0f;
    if (d < 1e-6f) { h = 0.0f; return; }
    if      (mx == r) h = 60.0f * (g - b) / d;
    else if (mx == g) h = 60.0f * (b - r) / d + 120.0f;
    else              h = 60.0f * (r - g) / d + 240.0f;
    if (h < 0.0f) h += 360.0f;
}

static void hsv2rgb(float h, float s, float v, float& r, float& g, float& b) {
    if (s < 1e-6f) { r = g = b = v; return; }
    h = std::fmod(h, 360.0f);
    if (h < 0.0f) h += 360.0f;
    int   i = static_cast<int>(h / 60.0f);
    float f = h / 60.0f - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));
    switch (i % 6) {
        case 0: r=v; g=t; b=p; break;
        case 1: r=q; g=v; b=p; break;
        case 2: r=p; g=v; b=t; break;
        case 3: r=p; g=q; b=v; break;
        case 4: r=t; g=p; b=v; break;
        default:r=v; g=p; b=q; break;
    }
}

void hueShift(Image& img, float degrees) {
    float* data = img.pixels.data();
    int px      = img.pixelCount();

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < px; ++i) {
        float* p = data + i * 4;
        float h, s, v;
        rgb2hsv(p[0], p[1], p[2], h, s, v);
        h += degrees;
        hsv2rgb(h, s, v, p[0], p[1], p[2]);
    }
}

// Vignette
// Pixel súlya: w = 1 - strength * dist^2
// ahol dist a normalizált távolság a képközéptől [0, 1]
void vignette(Image& img, float strength) {
    float* data = img.pixels.data();
    int    W    = img.width;
    int    H    = img.height;
    float  cx   = W * 0.5f;
    float  cy   = H * 0.5f;
    // maxD = cx*cx + cy*cy -- nincs sz\u00fcks\u00e9g r\u00e1, cx/cy-val normaliz\u00e1lunk

    #pragma omp parallel for collapse(2) schedule(static)
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float dx   = (x - cx) / cx;
            float dy   = (y - cy) / cy;
            float dist = dx * dx + dy * dy; // [0, 2]
            float w    = 1.0f - strength * dist * 0.5f;
            w = std::clamp(w, 0.0f, 1.0f);

            float* p = data + (y * W + x) * 4;
            p[0] *= w;
            p[1] *= w;
            p[2] *= w;
            // alpha meghagyva
        }
    }
}

} // namespace ContextFree
