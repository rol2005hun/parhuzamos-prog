#pragma once
#include <vector>
#include <string>
#include <cstdint>

// Image - RGBA float image (0.0-1.0 range)
// Load: stb_image, Save: stb_image_write (PNG)
struct Image {
    int width  = 0;
    int height = 0;
    // RGBA channels, interleaved: [R0,G0,B0,A0, R1,G1,B1,A1, ...]
    std::vector<float> pixels;

    bool empty() const { return pixels.empty(); }
    int  channels() const { return 4; }
    int  pixelCount() const { return width * height; }

    // Pixel access
    float* at(int x, int y) {
        return pixels.data() + (y * width + x) * 4;
    }
    const float* at(int x, int y) const {
        return pixels.data() + (y * width + x) * 4;
    }

    // File I/O
    bool load(const std::string& path);
    bool save(const std::string& path) const;

    // Clone
    Image clone() const;

    // uint8 conversion (GPU upload / display)
    std::vector<uint8_t> toUint8() const;
    void fromUint8(const uint8_t* data, int w, int h);
};
