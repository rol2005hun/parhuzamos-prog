#include "MandelbrotRenderer.h"
#include <glad/glad.h>
#include <iostream>
#include <cmath>

static const char* QUAD_VERT = R"(
#version 430 core
out vec2 v_uv;
void main() {
    vec2 pos = vec2((gl_VertexID & 1) * 2.0 - 1.0,
                    (gl_VertexID >> 1) * 2.0 - 1.0);
    v_uv = pos * 0.5 + 0.5;
    gl_Position = vec4(pos, 0.0, 1.0);
}
)";

static const char* QUAD_FRAG = R"(
#version 430 core
in vec2 v_uv;
out vec4 fragColor;
uniform sampler2D u_tex;
void main() { fragColor = texture(u_tex, vec2(v_uv.x, 1.0 - v_uv.y)); }
)";

MandelbrotRenderer::~MandelbrotRenderer() {
    if (m_tex) glDeleteTextures(1, &m_tex);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
}

bool MandelbrotRenderer::init() {
    if (!m_shMandelbrot.loadComputeFile("shaders/mandelbrot.comp")) return false;
    if (!m_shMandelbrotDouble.loadComputeFile("shaders/mandelbrot_double.comp")) return false;
    if (!m_shQuad.loadVF(QUAD_VERT, QUAD_FRAG)) return false;
    glGenVertexArrays(1, &m_vao);
    return true;
}

void MandelbrotRenderer::createTexture(int w, int h) {
    if (m_tex && m_texW == w && m_texH == h) return;
    if (m_tex) glDeleteTextures(1, &m_tex);
    glGenTextures(1, &m_tex);
    glBindTexture(GL_TEXTURE_2D, m_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glBindTexture(GL_TEXTURE_2D, 0);
    m_texW = w; m_texH = h;
}

void MandelbrotRenderer::render(int viewW, int viewH) {
    createTexture(viewW, viewH);

    GLShader& sh = useDouble ? m_shMandelbrotDouble : m_shMandelbrot;

    sh.use();
    glBindImageTexture(0, m_tex, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

    sh.setFloat2("u_resolution", (float)viewW, (float)viewH);
    if (useDouble) {
        sh.setDouble2("u_center",    m_cx, m_cy);
        sh.setDouble("u_zoom",       m_zoom);
        sh.setDouble2("u_juliaC",    (double)juliaRe, (double)juliaIm);
    } else {
        sh.setFloat2("u_center",     (float)m_cx, (float)m_cy);
        sh.setFloat ("u_zoom",       (float)m_zoom);
        sh.setFloat2("u_juliaC",     juliaRe, juliaIm);
    }
    sh.setInt   ("u_maxIter",    maxIter);
    sh.setInt   ("u_julia",      isJulia ? 1 : 0);
    sh.setInt   ("u_colorLUT",   colorLUT);

    uint32_t gx = (viewW + 15) / 16;
    uint32_t gy = (viewH + 15) / 16;
    sh.dispatch(gx, gy);
    GLShader::barrier();

    m_shQuad.use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_tex);
    m_shQuad.setInt("u_tex", 0);
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}

void MandelbrotRenderer::pan(double dx, double dy) {
    double scale = (2.0 * m_zoom) / m_texW;
    m_cx -= dx * scale;
    m_cy += dy * scale;
}

void MandelbrotRenderer::zoom(double factor, double cx, double cy) {
    // Zoom a kurzor köré
    double wx = (cx / m_texW  - 0.5) * 2.0 * m_zoom + m_cx;
    double wy = (cy / m_texH  - 0.5) * 2.0 * m_zoom * m_texH / m_texW + m_cy;
    m_zoom *= factor;
    m_cx = wx - (cx / m_texW  - 0.5) * 2.0 * m_zoom;
    m_cy = wy - (cy / m_texH  - 0.5) * 2.0 * m_zoom * m_texH / m_texW;
}

void MandelbrotRenderer::reset() {
    m_cx = -0.5; m_cy = 0.0; m_zoom = 1.5;
}
