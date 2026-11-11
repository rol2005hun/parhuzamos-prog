#pragma once
#include "../image/Image.h"
#include "../gpu/ComputeShader.h"
#include "../gpu/GLTexture.h"
#include <string>

// FilterMode - which context-sensitive filter is active
enum class FilterMode { None, Gauss, Sobel, Unsharp, Median };

// ExecutionMode - GPU or CPU
enum class ExecMode { GPU, CPU };

// App - main application class
class App {
public:
    App();
    ~App();

    bool init(int width = 1400, int height = 900);
    void run();

private:
    struct GLFWwindow* m_window = nullptr;
    int m_winW = 1400, m_winH = 900;

    Image m_original;         // original, unmodified image - NEVER changes
    Image m_processed;        // processed image (CPU pipeline)
    GLTexture m_texOriginal;  // always the original image (left preview)
    GLTexture m_texCFResult;  // after context-free filtering (GPU input)
    GLTexture m_texProcessed; // final result (right preview)
    GLTexture m_texTemp;      // intermediate texture (for two-pass Gauss)

    ComputeShader m_shGauss;
    ComputeShader m_shSobel;
    ComputeShader m_shUnsharp;
    ComputeShader m_shMedian;

    // Context-free parameters
    float m_brightness = 0.0f;
    float m_exposure   = 1.0f;
    float m_contrast   = 0.0f;
    float m_gamma      = 1.0f;
    float m_hue        = 0.0f;
    float m_vignette   = 0.0f;

    // Context-sensitive
    FilterMode m_filterMode = FilterMode::None;
    ExecMode   m_execMode   = ExecMode::GPU;
    int        m_kernelSize = 5;   // 3, 5, 7, 9
    float      m_unsharpAmt = 1.5f;

    // Performance measurement
    double m_lastGpuMs = 0.0;
    double m_lastCpuMs = 0.0;

    // Do we need to recalculate?
    bool m_dirty = false;
    bool m_imageLoaded = false;

    void initShaders();
    void loadImage(const std::string& path);
    void saveImage(const std::string& path);

    void applyFilters();
    void applyContextFreeFilters(Image& img);
    void applyContextSensitiveGPU();
    void applyContextSensitiveCPU(Image& img);

    void uploadTextures();
    void renderUI();
    void renderPreviews();

    // Helper: dispatch size (ceiled div)
    static uint32_t groups(int size, int local) {
        return (size + local - 1) / local;
    }
};
