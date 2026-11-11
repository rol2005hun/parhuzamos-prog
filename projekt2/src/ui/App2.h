#pragma once
#include "../mandelbrot/MandelbrotRenderer.h"
#include "../snowflake/KochGenerator.h"
#include "../snowflake/ParticleSystem.h"
#include "../histogram/Histogram.h"
#include "../gpu/GLShader.h"

struct GLFWwindow;

class App {
public:
    App();
    ~App();
    
    bool init();
    void run();

private:
    GLFWwindow* m_window = nullptr;
    int m_winW = 1400, m_winH = 900;
    
    void renderUI();
    void renderMandelbrotTab();
    void renderSnowflakeTab();
    void renderHistogramTab();
    
    // Tab 1: Mandelbrot
    MandelbrotRenderer m_mandelbrot;
    bool m_dragMandelbrot = false;
    double m_lastMouseX = 0, m_lastMouseY = 0;
    
    // Tab 2: Snowflake
    KochGenerator  m_koch;
    ParticleSystem m_particles;
    GLShader       m_shSnowflake;
    GLShader       m_shParticle;
    uint32_t       m_kochVAO = 0;
    uint32_t       m_kochVBO = 0;
    float          m_kochTime = 0.0f;
    double         m_lastTime = 0.0;
    
    // Snowflake FBO for ImGui integration
    uint32_t       m_snowFbo = 0;
    uint32_t       m_snowTex = 0;
    int            m_snowTexW = 0, m_snowTexH = 0;
    float          m_snowZoom = 1.0f;
    float          m_snowPanX = 0.0f, m_snowPanY = 0.0f;
    bool           m_dragSnow = false;
    void resizeSnowFbo(int w, int h);
    
    double         m_kochBenchSingle = 0.0;
    double         m_kochBenchMulti  = 0.0;
    void runKochBenchmark();
    
    void updateKochVBO();
    
    // Tab 3: Histogram
    std::vector<float> m_histImage; // Dummy RGBA image data for benchmarking
    int m_histImgW = 1920;
    int m_histImgH = 1080;
    int m_histSizeIdx = 1; // 0=HD, 1=FHD, 2=4K, 3=8K
    std::vector<HistogramResult> m_histResults;
    bool m_histComputed = false;
    
    void runHistogramBenchmark();
    void generateHistogramData();
};
