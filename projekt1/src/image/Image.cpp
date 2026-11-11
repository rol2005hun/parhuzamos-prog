#include "Image.h"
#include <algorithm>
#include <cstring>

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image.h"
#include "stb/stb_image_write.h"

bool Image::load(const std::string& path) {
    int w, h, c;
    // Do NOT flip vertically: ImGui and stb_image_write expect row 0 to be the top
    stbi_set_flip_vertically_on_load(false);
    uint8_t* data = stbi_load(path.c_str(), &w, &h, &c, 4);
    if (!data) return false;

    fromUint8(data, w, h);
    stbi_image_free(data);
    return true;
}

bool Image::save(const std::string& path) const {
    auto u8 = toUint8();
    return stbi_write_png(path.c_str(), width, height, 4, u8.data(), width * 4) != 0;
}

Image Image::clone() const {
    Image img;
    img.width  = width;
    img.height = height;
    img.pixels = pixels;
    return img;
}

std::vector<uint8_t> Image::toUint8() const {
    std::vector<uint8_t> out(pixels.size());
    for (size_t i = 0; i < pixels.size(); ++i) {
        float v = std::clamp(pixels[i], 0.0f, 1.0f);
        out[i] = static_cast<uint8_t>(v * 255.0f + 0.5f);
    }
    return out;
}

void Image::fromUint8(const uint8_t* data, int w, int h) {
    width  = w;
    height = h;
    pixels.resize(w * h * 4);
    constexpr float inv255 = 1.0f / 255.0f;
    for (size_t i = 0; i < pixels.size(); ++i) {
        pixels[i] = data[i] * inv255;
    }
}
