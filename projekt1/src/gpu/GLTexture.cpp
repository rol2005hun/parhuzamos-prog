#include "GLTexture.h"
#include <glad/glad.h>
#include <vector>

GLTexture::~GLTexture() {
    if (m_id) glDeleteTextures(1, &m_id);
}

void GLTexture::createIfNeeded(int w, int h) {
    if (m_id && m_w == w && m_h == h) return;
    if (m_id) glDeleteTextures(1, &m_id);

    glGenTextures(1, &m_id);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // RGBA32F internal format - required for compute shader image2D
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);

    m_w = w;
    m_h = h;
}

void GLTexture::upload(const Image& img) {
    createIfNeeded(img.width, img.height);
    glBindTexture(GL_TEXTURE_2D, m_id);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, img.width, img.height,
                    GL_RGBA, GL_FLOAT, img.pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
}

Image GLTexture::download() const {
    Image img;
    img.width  = m_w;
    img.height = m_h;
    img.pixels.resize(m_w * m_h * 4);

    glBindTexture(GL_TEXTURE_2D, m_id);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, img.pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);
    return img;
}

void GLTexture::bindImage(uint32_t unit) const {
    // GL_READ_WRITE + rgba32f: layout(rgba32f) image2D in compute shader
    glBindImageTexture(unit, m_id, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
}

void GLTexture::bindTexture(uint32_t unit) const {
    glActiveTexture(GL_TEXTURE0 + unit);
    glBindTexture(GL_TEXTURE_2D, m_id);
}
