#pragma once
#include "../image/Image.h"
#include <cstdint>

// GLTexture - OpenGL RGBA32F texture wrapper
// bindable as an image unit in compute shaders (layout = rgba32f)
class GLTexture {
public:
    GLTexture() = default;
    ~GLTexture();

    // Non-copyable
    GLTexture(const GLTexture&) = delete;
    GLTexture& operator=(const GLTexture&) = delete;

    // Movable
    GLTexture(GLTexture&& o) noexcept : m_id(o.m_id), m_w(o.m_w), m_h(o.m_h) {
        o.m_id = 0;
    }

    // Create / update from CPU Image
    void upload(const Image& img);

    // Read from GPU to CPU Image
    Image download() const;

    // Binding: image unit (read_write), texture unit (sample)
    void bindImage(uint32_t unit) const;
    void bindTexture(uint32_t unit) const;

    uint32_t id()     const { return m_id; }
    int      width()  const { return m_w; }
    int      height() const { return m_h; }
    bool     valid()  const { return m_id != 0; }

private:
    uint32_t m_id = 0;
    int      m_w  = 0;
    int      m_h  = 0;

    void createIfNeeded(int w, int h);
};
