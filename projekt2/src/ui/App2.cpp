#include "App2.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <random>
#include <omp.h>
#include <chrono>

App::App() {}

App::~App() {
    if (m_kochVAO) glDeleteVertexArrays(1, &m_kochVAO);
    if (m_kochVBO) glDeleteBuffers(1, &m_kochVBO);
    m_particles.destroy();
    if (m_snowFbo) glDeleteFramebuffers(1, &m_snowFbo);
    if (m_snowTex) glDeleteTextures(1, &m_snowTex);
    
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    if (m_window) glfwDestroyWindow(m_window);
    glfwTerminate();
}

bool App::init() {
    if (!glfwInit()) return false;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    m_window = glfwCreateWindow(m_winW, m_winH, "Projekt 2: Mandelbrot & Snowflake", nullptr, nullptr);
    if (!m_window) return false;
    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // VSync
    
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return false;
    
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 430");
    
    // Mandelbrot init
    if (!m_mandelbrot.init()) return false;
    
    // Snowflake init
    if (!m_shSnowflake.loadVFFiles("shaders/snowflake.vert", "shaders/snowflake.frag")) return false;
    if (!m_shParticle.loadVFFiles("shaders/particle.vert", "shaders/particle.frag")) return false;
    glGenVertexArrays(1, &m_kochVAO);
    glGenBuffers(1, &m_kochVBO);
    m_particles.init(5000);
    m_koch.generate();
    updateKochVBO();
    m_lastTime = glfwGetTime();
    
    // Histogram dummy data gen
    generateHistogramData();
    
    return true;
}

void App::generateHistogramData() {
    m_histImage.resize(m_histImgW * m_histImgH * 4);
    std::mt19937 rng(1337);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    for (float& f : m_histImage) f = dist(rng);
    m_histComputed = false;
}

void App::resizeSnowFbo(int w, int h) {
    if (m_snowFbo != 0 && m_snowTexW == w && m_snowTexH == h) return;
    
    if (m_snowFbo == 0) glGenFramebuffers(1, &m_snowFbo);
    if (m_snowTex != 0) glDeleteTextures(1, &m_snowTex);
    
    glGenTextures(1, &m_snowTex);
    glBindTexture(GL_TEXTURE_2D, m_snowTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    glBindFramebuffer(GL_FRAMEBUFFER, m_snowFbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_snowTex, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    
    m_snowTexW = w;
    m_snowTexH = h;
}

void App::updateKochVBO() {
    glBindVertexArray(m_kochVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_kochVBO);
    glBufferData(GL_ARRAY_BUFFER, m_koch.vertices().size() * sizeof(Vec2), m_koch.vertices().data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void App::runKochBenchmark() {
    auto t0 = std::chrono::high_resolution_clock::now();
    omp_set_num_threads(1);
    m_koch.generate();
    auto t1 = std::chrono::high_resolution_clock::now();
    
    omp_set_num_threads(omp_get_num_procs());
    m_koch.generate();
    auto t2 = std::chrono::high_resolution_clock::now();
    
    m_kochBenchSingle = std::chrono::duration<double, std::milli>(t1 - t0).count();
    m_kochBenchMulti = std::chrono::duration<double, std::milli>(t2 - t1).count();
    updateKochVBO();
}

void App::runHistogramBenchmark() {
    m_histResults.clear();
    const float* data = m_histImage.data();
    int pixels = m_histImgW * m_histImgH;
    
    m_histResults.push_back(Histogram::computeSingle(data, pixels));
    m_histResults.push_back(Histogram::computeMultiAtomic1(data, pixels));
    m_histResults.push_back(Histogram::computeMultiAtomic3(data, pixels));
    m_histResults.push_back(Histogram::computeMultiLocal(data, pixels));
    m_histComputed = true;
}

void App::run() {
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        
        double current_time = glfwGetTime();
        float dt = (float)(current_time - m_lastTime);
        m_lastTime = current_time;
        m_kochTime += dt;
        
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        renderUI();
        
        ImGui::Render();
        glfwGetFramebufferSize(m_window, &m_winW, &m_winH);
        glViewport(0, 0, m_winW, m_winH);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        
        glfwSwapBuffers(m_window);
    }
}

void App::renderUI() {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2((float)m_winW, (float)m_winH));
    ImGui::Begin("Main", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus);
    
    if (ImGui::BeginTabBar("Tabs")) {
        if (ImGui::BeginTabItem("Mandelbrot / Julia")) {
            renderMandelbrotTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Koch Snowflake")) {
            renderSnowflakeTab();
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Histogram Benchmark")) {
            renderHistogramTab();
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    
    ImGui::Separator();
    const char* gpuName = (const char*)glGetString(GL_RENDERER);
    ImGui::TextDisabled("Multithreading: OpenMP  |  GPGPU: OpenGL 4.3 Compute Shader  |  %s", gpuName ? gpuName : "Unknown GPU");
    
    ImGui::End();
}

void App::renderMandelbrotTab() {
    ImGui::BeginChild("MandelCtrl", ImVec2(300, 0), true);
    ImGui::Text("GPU Compute Render");
    ImGui::Separator();
    ImGui::Checkbox("Julia Set", &m_mandelbrot.isJulia);
    if (m_mandelbrot.isJulia) {
        ImGui::SliderFloat("Re", &m_mandelbrot.juliaRe, -2.0f, 2.0f);
        ImGui::SliderFloat("Im", &m_mandelbrot.juliaIm, -2.0f, 2.0f);
    }
    ImGui::SliderInt("Max Iter", &m_mandelbrot.maxIter, 64, 2048);
    ImGui::Checkbox("64-bit Precision (Slower!)", &m_mandelbrot.useDouble);
    const char* palettes[] = { "Smooth", "Fire", "Ice", "Neon", "Grayscale" };
    ImGui::Combo("Palette", &m_mandelbrot.colorLUT, palettes, 5);
    if (ImGui::Button("Reset View")) m_mandelbrot.reset();
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    ImGui::BeginChild("MandelView", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImVec2 viewSize = ImGui::GetContentRegionAvail();
    if (viewSize.x > 0 && viewSize.y > 0) {
        m_mandelbrot.render((int)viewSize.x, (int)viewSize.y);
        
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImGui::Image((ImTextureID)(intptr_t)m_mandelbrot.textureId(), viewSize, ImVec2(0, 1), ImVec2(1, 0));
        
        // Navigation logic
        ImGuiIO& io = ImGui::GetIO();
        if (ImGui::IsItemHovered()) {
            if (io.MouseWheel != 0.0f) {
                float factor = io.MouseWheel > 0 ? 0.9f : 1.1f;
                m_mandelbrot.zoom(factor, io.MousePos.x - pos.x, io.MousePos.y - pos.y);
            }
            if (ImGui::IsMouseClicked(0)) m_dragMandelbrot = true;
        }
        if (m_dragMandelbrot) {
            if (ImGui::IsMouseDragging(0)) {
                m_mandelbrot.pan(io.MouseDelta.x, io.MouseDelta.y);
            }
            if (ImGui::IsMouseReleased(0)) m_dragMandelbrot = false;
        }
    }
    ImGui::EndChild();
}

void App::renderSnowflakeTab() {
    ImGui::BeginChild("SnowCtrl", ImVec2(300, 0), true);
    ImGui::Text("Koch Snowflake Settings");
    bool changed = false;
    changed |= ImGui::SliderInt("Depth", &m_koch.depth, 1, 10); // Now goes up to 10
    changed |= ImGui::SliderFloat("Angle", &m_koch.angle, 30.0f, 120.0f);
    changed |= ImGui::SliderFloat("Ratio", &m_koch.ratio, 0.1f, 0.9f);
    changed |= ImGui::Checkbox("Asymmetric", &m_koch.asymmetric);
    if (changed) { m_koch.generate(); updateKochVBO(); }
    
    ImGui::Separator();
    ImGui::Text("Particle System");
    ImGui::SliderFloat("Spawn Rate", &m_particles.spawnRate, 10.0f, 200.0f);
    ImGui::SliderFloat("Gravity", &m_particles.gravity, -0.2f, 0.2f);
    ImGui::SliderFloat("Wind", &m_particles.wind, -0.2f, 0.2f);
    ImGui::Text("Active Particles: %d", m_particles.activeCount());
    
    ImGui::Separator();
    ImGui::Text("Performance Benchmark");
    if (ImGui::Button("Run Benchmark", ImVec2(-1, 0))) {
        runKochBenchmark();
    }
    if (m_kochBenchSingle > 0.0) {
        ImGui::Text("CPU 1 Core: %.2f ms", m_kochBenchSingle);
        ImGui::Text("CPU All Cores: %.2f ms", m_kochBenchMulti);
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Speedup: %.2fx", m_kochBenchSingle / m_kochBenchMulti);
    }
    ImGui::EndChild();
    
    ImGui::SameLine();
    
    ImGui::BeginChild("SnowView", ImVec2(0, 0), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImVec2 size = ImGui::GetContentRegionAvail();
    
    if (size.x > 0 && size.y > 0) {
        resizeSnowFbo((int)size.x, (int)size.y);
        
        // Update particles (Parallel)
        m_particles.update(ImGui::GetIO().DeltaTime, m_koch.tips());
        m_particles.uploadToGPU();
        
        // Render to FBO
        glBindFramebuffer(GL_FRAMEBUFFER, m_snowFbo);
        glViewport(0, 0, (int)size.x, (int)size.y);
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f); // Slightly bluish background for the snow
        glClear(GL_COLOR_BUFFER_BIT);
        
        // Render lines
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        
        m_shSnowflake.use();
        m_shSnowflake.setFloat("u_time", m_kochTime);
        m_shSnowflake.setFloat3("u_color", 0.7f, 0.9f, 1.0f);
        m_shSnowflake.setFloat("u_zoom", m_snowZoom);
        m_shSnowflake.setFloat2("u_pan", m_snowPanX, m_snowPanY);
        glBindVertexArray(m_kochVAO);
        glDrawArrays(GL_LINE_STRIP, 0, m_koch.vertices().size());
        
        // Render particles
        glEnable(GL_PROGRAM_POINT_SIZE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive blend for particles
        m_shParticle.use();
        m_shParticle.setFloat("u_zoom", m_snowZoom);
        m_shParticle.setFloat2("u_pan", m_snowPanX, m_snowPanY);
        glBindVertexArray(m_particles.vao);
        glDrawArrays(GL_POINTS, 0, m_particles.activeCount());
        
        glBindVertexArray(0);
        glDisable(GL_PROGRAM_POINT_SIZE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Reset blend
        
        glBindFramebuffer(GL_FRAMEBUFFER, 0); // Vissza az alaphoz
        
        // Draw FBO texture in ImGui
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImGui::Image((ImTextureID)(intptr_t)m_snowTex, size, ImVec2(0, 1), ImVec2(1, 0));
        
        // Navigation logic for Snowflake
        ImGuiIO& io = ImGui::GetIO();
        if (ImGui::IsItemHovered()) {
            if (io.MouseWheel != 0.0f) {
                // Zoom
                float factor = io.MouseWheel > 0 ? 1.1f : 0.9f;
                // Cursor position inside the image (approx NDC coordinates)
                float mx = (io.MousePos.x - pos.x) / size.x * 2.0f - 1.0f;
                float my = -((io.MousePos.y - pos.y) / size.y * 2.0f - 1.0f);
                
                // Zoom around cursor
                m_snowPanX += mx * (1.0f - factor) / m_snowZoom;
                m_snowPanY += my * (1.0f - factor) / m_snowZoom;
                m_snowZoom *= factor;
            }
            if (ImGui::IsMouseClicked(0)) m_dragSnow = true;
        }
        if (m_dragSnow) {
            if (ImGui::IsMouseDragging(0)) {
                m_snowPanX += io.MouseDelta.x / (size.x * 0.5f) / m_snowZoom;
                m_snowPanY -= io.MouseDelta.y / (size.y * 0.5f) / m_snowZoom;
            }
            if (ImGui::IsMouseReleased(0)) m_dragSnow = false;
        }
    }
    
    ImGui::EndChild();
}

void App::renderHistogramTab() {
    ImGui::BeginChild("HistCtrl", ImVec2(300, 0), true);
    ImGui::Text("Histogram Benchmark");
    
    const char* sizes[] = { "1280x720 (HD)", "1920x1080 (FHD)", "3840x2160 (4K)", "7680x4320 (8K)" };
    if (ImGui::Combo("Resolution", &m_histSizeIdx, sizes, 4)) {
        if (m_histSizeIdx == 0) { m_histImgW = 1280; m_histImgH = 720; }
        else if (m_histSizeIdx == 1) { m_histImgW = 1920; m_histImgH = 1080; }
        else if (m_histSizeIdx == 2) { m_histImgW = 3840; m_histImgH = 2160; }
        else if (m_histSizeIdx == 3) { m_histImgW = 7680; m_histImgH = 4320; }
        generateHistogramData();
    }
    
    ImGui::Text("Image: %dx%d\n(%.1f millió pixel)", m_histImgW, m_histImgH, (m_histImgW * m_histImgH) / 1000000.0f);
    ImGui::Separator();
    if (ImGui::Button("Run Benchmark", ImVec2(-1, 0))) {
        runHistogramBenchmark();
    }
    ImGui::EndChild();
    
    ImGui::SameLine();
    ImGui::BeginChild("HistView", ImVec2(0, 0), true);
    if (!m_histComputed) {
        ImGui::Text("Press Run Benchmark...");
    } else {
        for (const auto& res : m_histResults) {
            ImGui::Text("%s: %.2f ms", res.label.c_str(), res.timeMs);
            // Quick ASCII bar chart for R channel just as a sanity check
            // A real graph would use ImGui::PlotLines or ImPlot
        }
        
        ImGui::Separator();
        ImGui::Text("Red Channel Histogram (Local+Sum output)");
        if (!m_histResults.empty()) {
            const auto& finalRes = m_histResults.back();
            // Convert to float for PlotHistogram
            std::vector<float> rF(256);
            for(int i=0; i<256; i++) rF[i] = (float)finalRes.r[i];
            ImGui::PlotHistogram("##R", rF.data(), 256, 0, nullptr, 0.0f, FLT_MAX, ImVec2(0, 150));
        }
    }
    ImGui::EndChild();
}
