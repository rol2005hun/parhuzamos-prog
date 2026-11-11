#include "App.h"
#include "../image/ContextFree.h"
#include "../image/ContextSensitive.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <iostream>
#include <chrono>
#include <algorithm>
#include <cmath>

// GLFW callback
static void glfw_error(int e, const char* d) {
    std::cerr << "[GLFW] Error " << e << ": " << d << "\n";
}

App::App()  = default;
App::~App() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (m_window) glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool App::init(int width, int height) {
    m_winW = width; m_winH = height;

    glfwSetErrorCallback(glfw_error);
    if (!glfwInit()) return false;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    m_window = glfwCreateWindow(width, height, "Parallel Image Processor - Project 1", nullptr, nullptr);
    if (!m_window) return false;

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // VSync

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "[GLAD] Failed to initialize\n";
        return false;
    }
    std::cout << "[OpenGL] " << glGetString(GL_VERSION) << "\n";
    std::cout << "[GPU]    " << glGetString(GL_RENDERER) << "\n";

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    // Dark, modern style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding    = 8.0f;
    style.FrameRounding     = 5.0f;
    style.GrabRounding      = 4.0f;
    style.ItemSpacing       = ImVec2(8, 6);
    style.FramePadding      = ImVec2(8, 5);
    style.Colors[ImGuiCol_WindowBg]         = ImVec4(0.10f, 0.10f, 0.13f, 1.00f);
    style.Colors[ImGuiCol_Header]           = ImVec4(0.25f, 0.25f, 0.60f, 0.80f);
    style.Colors[ImGuiCol_HeaderHovered]    = ImVec4(0.30f, 0.30f, 0.75f, 0.90f);
    style.Colors[ImGuiCol_Button]           = ImVec4(0.25f, 0.40f, 0.80f, 0.85f);
    style.Colors[ImGuiCol_ButtonHovered]    = ImVec4(0.35f, 0.50f, 0.90f, 1.00f);
    style.Colors[ImGuiCol_SliderGrab]       = ImVec4(0.40f, 0.60f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.50f, 0.70f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_FrameBg]          = ImVec4(0.15f, 0.15f, 0.20f, 1.00f);
    style.Colors[ImGuiCol_TitleBg]          = ImVec4(0.08f, 0.08f, 0.12f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]    = ImVec4(0.15f, 0.15f, 0.30f, 1.00f);

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 430");

    initShaders();
    loadImage("assets/test.png");
    return true;
}

void App::initShaders() {
    if (!m_shGauss.loadFromFile("shaders/gauss.comp"))
        std::cerr << "[App] Failed to load Gauss shader!\n";
    if (!m_shSobel.loadFromFile("shaders/sobel.comp"))
        std::cerr << "[App] Failed to load Sobel shader!\n";
    if (!m_shUnsharp.loadFromFile("shaders/unsharp.comp"))
        std::cerr << "[App] Failed to load Unsharp shader!\n";
    if (!m_shMedian.loadFromFile("shaders/median.comp"))
        std::cerr << "[App] Failed to load Median shader!\n";
}

void App::loadImage(const std::string& path) {
    if (m_original.load(path)) {
        m_processed   = m_original.clone();
        m_imageLoaded = true;
        m_dirty       = true;
        // The original texture is uploaded once and NEVER changes
        m_texOriginal.upload(m_original);
        // Allocate sizes for other textures
        m_texCFResult.upload(m_original);
        m_texTemp.upload(m_original);
        m_texProcessed.upload(m_original);
        std::cout << "[App] Image loaded: " << path
                  << " (" << m_original.width << "x" << m_original.height << ")\n";
    } else {
        std::cerr << "[App] Failed to load image: " << path << "\n";
    }
}

void App::saveImage(const std::string& path) {
    if (!m_imageLoaded) return;
    if (m_execMode == ExecMode::GPU && m_filterMode != FilterMode::None) {
        // Download GPU result
        m_processed = m_texProcessed.download();
    }
    if (m_processed.save(path))
        std::cout << "[App] Image saved: " << path << "\n";
    else
        std::cerr << "[App] Failed to save image: " << path << "\n";
}

// Filter pipeline
void App::applyFilters() {
    if (!m_imageLoaded) return;

    // 1. Context-free filters (CPU SIMD+OpenMP) - work on a clone
    m_processed = m_original.clone();
    applyContextFreeFilters(m_processed);

    // Upload CF result to the GPU input texture (NOT to original!)
    m_texCFResult.upload(m_processed);

    // If there is no context-sensitive filter, the CF result goes to the right panel
    if (m_filterMode == FilterMode::None) {
        m_texProcessed.upload(m_processed);
        return;
    }

    // 2. Context-sensitive filter
    if (m_execMode == ExecMode::GPU)
        applyContextSensitiveGPU();
    else
        applyContextSensitiveCPU(m_processed);
}

void App::applyContextFreeFilters(Image& img) {
    if (m_brightness != 0.0f)   ContextFree::brightness(img, m_brightness);
    if (m_exposure   != 1.0f)   ContextFree::exposure(img, m_exposure);
    if (m_contrast   != 0.0f)   ContextFree::contrast(img, m_contrast);
    if (m_gamma      != 1.0f)   ContextFree::gamma(img, m_gamma);
    if (m_hue        != 0.0f)   ContextFree::hueShift(img, m_hue);
    if (m_vignette   != 0.0f)   ContextFree::vignette(img, m_vignette);
}

void App::applyContextSensitiveGPU() {
    int W = m_original.width;
    int H = m_original.height;
    uint32_t gx = groups(W, 16);
    uint32_t gy = groups(H, 16);

    auto t0 = std::chrono::high_resolution_clock::now();

    switch (m_filterMode) {
    case FilterMode::Gauss: {
        // Pass 1: horizontal (input -> temp)
        m_shGauss.use();
        m_shGauss.setInt("u_kernelSize", m_kernelSize);
        m_shGauss.setInt("u_horizontal", 1);
        m_texCFResult.bindImage(0);  // image after CF (input)
        m_texTemp.bindImage(1);
        m_shGauss.dispatch(gx, gy);
        ComputeShader::barrier();
        // Pass 2: vertical (temp -> output)
        m_shGauss.setInt("u_horizontal", 0);
        m_texTemp.bindImage(0);
        m_texProcessed.bindImage(1);
        m_shGauss.dispatch(gx, gy);
        ComputeShader::barrier();
        break;
    }
    case FilterMode::Sobel: {
        m_shSobel.use();
        m_texCFResult.bindImage(0);  // image after CF (input)
        m_texProcessed.bindImage(1);
        m_shSobel.dispatch(gx, gy);
        ComputeShader::barrier();
        break;
    }
    case FilterMode::Unsharp: {
        // Blur pass 1: horizontal - CFResult -> temp
        m_shGauss.use();
        m_shGauss.setInt("u_kernelSize", m_kernelSize);
        m_shGauss.setInt("u_horizontal", 1);
        m_texCFResult.bindImage(0);
        m_texTemp.bindImage(1);
        m_shGauss.dispatch(gx, gy);
        ComputeShader::barrier();
        // Blur pass 2: vertical - temp -> processed
        m_shGauss.setInt("u_horizontal", 0);
        m_texTemp.bindImage(0);
        m_texProcessed.bindImage(1);
        m_shGauss.dispatch(gx, gy);
        ComputeShader::barrier();
        // Unsharp: CFResult (sharp) - processed (blur) -> temp
        m_shUnsharp.use();
        m_shUnsharp.setFloat("u_amount", m_unsharpAmt);
        m_texCFResult.bindImage(0);   // original (after CF)
        m_texProcessed.bindImage(1);  // blurred
        m_texTemp.bindImage(2);       // output
        m_shUnsharp.dispatch(gx, gy);
        ComputeShader::barrier();
        // temp -> processed (final result to the right panel)
        Image tmp = m_texTemp.download();
        m_texProcessed.upload(tmp);
        break;
    }
    case FilterMode::Median: {
        m_shMedian.use();
        m_shMedian.setInt("u_kernelSize", m_kernelSize);
        m_texCFResult.bindImage(0);  // image after CF (input)
        m_texProcessed.bindImage(1);
        m_shMedian.dispatch(gx, gy);
        ComputeShader::barrier();
        break;
    }
    default: break;
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    m_lastGpuMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
}

void App::applyContextSensitiveCPU(Image& img) {
    auto t0 = std::chrono::high_resolution_clock::now();

    switch (m_filterMode) {
    case FilterMode::Gauss:
        img = ContextSensitive::gaussBlur(img, m_kernelSize);
        break;
    case FilterMode::Sobel:
        img = ContextSensitive::edgeDetect(img);
        break;
    case FilterMode::Unsharp:
        img = ContextSensitive::unsharpMask(img, m_kernelSize, m_unsharpAmt);
        break;
    case FilterMode::Median:
        img = ContextSensitive::medianFilter(img, m_kernelSize);
        break;
    default: break;
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    m_lastCpuMs = std::chrono::duration<double, std::milli>(t1 - t0).count();

    m_texProcessed.upload(img);
}

// Main render loop
void App::run() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

        // If something changed
        if (m_dirty && m_imageLoaded) {
            applyFilters();
            m_dirty = false;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        glfwGetWindowSize(m_window, &m_winW, &m_winH);
        glClearColor(0.07f, 0.07f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        renderUI();

        ImGui::Render();
        glViewport(0, 0, m_winW, m_winH);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(m_window);
    }
}

// UI
void App::renderUI() {
    // Full window covering panel
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)m_winW, (float)m_winH));
    ImGui::Begin("##main", nullptr,
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoScrollbar);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.8f, 1.0f, 1.0f));
    ImGui::SetWindowFontScale(1.4f);
    ImGui::Text("  Parallel Image Processor");
    ImGui::SetWindowFontScale(1.0f);
    ImGui::PopStyleColor();
    ImGui::Text("  SIMD (AVX2) + OpenMP + OpenGL Compute Shader | Project 1");
    ImGui::Separator();

    static char loadPath[512] = "assets/test.png";
    static char savePath[512] = "output.png";

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.9f, 0.9f, 1.0f));
    ImGui::Text("File:");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::SetNextItemWidth(350);
    ImGui::InputText("##loadpath", loadPath, sizeof(loadPath));
    ImGui::SameLine();
    if (ImGui::Button("  Load  ")) {
        loadImage(loadPath);
    }
    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("##savepath", savePath, sizeof(savePath));
    ImGui::SameLine();
    if (ImGui::Button("  Save  ")) {
        saveImage(savePath);
    }
    ImGui::Separator();

    float panelW = 340.0f;
    float previewH = (float)m_winH - 220.0f;

    ImGui::BeginChild("##controls", ImVec2(panelW, previewH), false);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.9f, 0.7f, 1.0f));
    ImGui::Text("▶ Context-free (SIMD + OpenMP)");
    ImGui::PopStyleColor();
    ImGui::Separator();

    bool cf_changed = false;
    cf_changed |= ImGui::SliderFloat("Brightness",  &m_brightness, -1.0f,  1.0f);
    cf_changed |= ImGui::SliderFloat("Exposure",    &m_exposure,    0.1f,  5.0f);
    cf_changed |= ImGui::SliderFloat("Contrast",    &m_contrast,   -1.0f,  1.0f);
    cf_changed |= ImGui::SliderFloat("Gamma",       &m_gamma,       0.1f,  5.0f);
    cf_changed |= ImGui::SliderFloat("Hue Shift°",  &m_hue,       -180.0f,180.0f);
    cf_changed |= ImGui::SliderFloat("Vignette",    &m_vignette,    0.0f,  1.0f);
    if (cf_changed) m_dirty = true;

    if (ImGui::Button("Reset##cf")) {
        m_brightness = 0.0f; m_exposure = 1.0f; m_contrast = 0.0f;
        m_gamma = 1.0f;      m_hue = 0.0f;      m_vignette = 0.0f;
        m_dirty = true;
    }

    ImGui::Dummy(ImVec2(0, 12));

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.75f, 0.4f, 1.0f));
    ImGui::Text("▶ Context-sensitive (Compute Shader / OpenMP)");
    ImGui::PopStyleColor();
    ImGui::Separator();

    // Execution mode
    ImGui::Text("Mode:");
    ImGui::SameLine();
    bool gpuMode = (m_execMode == ExecMode::GPU);
    if (ImGui::RadioButton("GPU##mode", gpuMode)) { m_execMode = ExecMode::GPU; m_dirty = true; }
    ImGui::SameLine();
    if (ImGui::RadioButton("CPU##mode", !gpuMode)) { m_execMode = ExecMode::CPU; m_dirty = true; }

    ImGui::Dummy(ImVec2(0, 4));

    auto filterBtn = [&](const char* label, FilterMode mode) {
        bool active = (m_filterMode == mode);
        if (active) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.6f, 1.0f, 1.0f));
        if (ImGui::Button(label, ImVec2(150, 30))) {
            m_filterMode = (m_filterMode == mode) ? FilterMode::None : mode;
            m_dirty = true;
        }
        if (active) ImGui::PopStyleColor();
    };

    filterBtn("Gauss Blur",     FilterMode::Gauss);
    ImGui::SameLine();
    filterBtn("Edge Detection", FilterMode::Sobel);
    filterBtn("Unsharp Mask",   FilterMode::Unsharp);
    ImGui::SameLine();
    filterBtn("Median Filter",  FilterMode::Median);

    if (m_filterMode != FilterMode::None && m_filterMode != FilterMode::Sobel) {
        ImGui::Dummy(ImVec2(0, 4));
        ImGui::Text("Kernel size:");
        ImGui::SameLine();
        int ks = m_kernelSize;
        bool changed = false;
        if (ImGui::RadioButton("3##k", ks==3)){ks=3;changed=true;}
        ImGui::SameLine();
        if (ImGui::RadioButton("5##k", ks==5)){ks=5;changed=true;}
        ImGui::SameLine();
        if (ImGui::RadioButton("7##k", ks==7)){ks=7;changed=true;}
        ImGui::SameLine();
        if (ImGui::RadioButton("9##k", ks==9)){ks=9;changed=true;}
        if (changed) { m_kernelSize = ks; m_dirty = true; }

        if (m_filterMode == FilterMode::Unsharp) {
            if (ImGui::SliderFloat("Amount", &m_unsharpAmt, 0.5f, 3.0f)) m_dirty = true;
        }
    }

    ImGui::Dummy(ImVec2(0, 12));

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.5f, 1.0f));
    ImGui::Text("▶ Performance");
    ImGui::PopStyleColor();
    ImGui::Separator();

    if (m_execMode == ExecMode::GPU) {
        ImGui::Text("GPU time: %.2f ms", m_lastGpuMs);
    } else {
        ImGui::Text("CPU time: %.2f ms", m_lastCpuMs);
    }
    if (m_lastCpuMs > 0.0 && m_lastGpuMs > 0.0) {
        double speedup = m_lastCpuMs / m_lastGpuMs;
        ImGui::Text("Speedup (CPU/GPU): %.1f×", speedup);
    }

    if (m_imageLoaded) {
        ImGui::Text("Image: %d x %d px", m_original.width, m_original.height);
    }

    ImGui::EndChild();

    ImGui::SameLine();
    ImGui::BeginChild("##previews", ImVec2(0, previewH), false);

    float availW = ImGui::GetContentRegionAvail().x;
    float imgW   = (availW - 20.0f) * 0.5f;

    if (m_imageLoaded) {
        float aspect = (float)m_original.height / m_original.width;
        float imgH   = imgW * aspect;

        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        ImGui::Text("  Original");
        ImGui::PopStyleColor();
        ImGui::SameLine(imgW + 20.0f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
        ImGui::Text("  Processed");
        ImGui::PopStyleColor();

        // Cast OpenGL texture ID for ImGui
        ImTextureID origTex = (ImTextureID)(intptr_t)m_texOriginal.id();
        ImTextureID procTex = (ImTextureID)(intptr_t)m_texProcessed.id();

        ImGui::Image(origTex, ImVec2(imgW, imgH));
        ImGui::SameLine();
        ImGui::Image(procTex, ImVec2(imgW, imgH));
    } else {
        ImGui::Dummy(ImVec2(20, 20));
        ImGui::TextDisabled("  Load an image to display.");
        ImGui::TextDisabled("  Recommended: assets/test.png");
    }

    ImGui::EndChild();

    ImGui::Separator();
    const char* gpuName = (const char*)glGetString(GL_RENDERER);
    ImGui::TextDisabled("SIMD: AVX2  |  Multithreading: OpenMP  |  GPGPU: OpenGL 4.3 Compute Shader  |  %s", gpuName ? gpuName : "Unknown GPU");

    ImGui::End();
}
