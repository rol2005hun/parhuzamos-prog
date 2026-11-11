#pragma once
#include "../gpu/GLShader.h"
#include <cstdint>
#include <functional>

// MandelbrotRenderer — GPU compute shader alapú Mandelbrot/Julia renderer
// Kimenet: RGBA8 textúra (displayre)
class MandelbrotRenderer {
public:
    MandelbrotRenderer() = default;
    ~MandelbrotRenderer();

    bool init();
    // Render a jelenlegi paraméterekkel
    void render(int viewW, int viewH);

    // navigation
    void pan(double dx, double dy);   // pixel delta
    void zoom(double factor, double cx, double cy); // scroll
    void reset();

    // parameters
    bool  isJulia    = false;
    float juliaRe    = -0.7f;
    float juliaIm    =  0.27f;
    int   maxIter    = 256;
    int   colorLUT   = 0;   // 0–4: paletta index
    bool  useDouble  = true;

    // texture id for imgui
    uint32_t textureId() const { return m_tex; }

private:
    GLShader  m_shMandelbrot;
    GLShader  m_shMandelbrotDouble;
    GLShader  m_shQuad;       // fullscreen quad megjelenítő
    uint32_t  m_tex   = 0;
    uint32_t  m_vao   = 0;
    int       m_texW  = 0, m_texH = 0;

    // view state
    double m_cx   = -0.5, m_cy = 0.0;   // középpont komplex síkon
    double m_zoom =  1.5;                // sugár (komplex egység)

    void createTexture(int w, int h);
};
