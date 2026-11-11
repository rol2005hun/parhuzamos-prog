#include "ComputeShader.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>

ComputeShader::~ComputeShader() {
    if (m_id) glDeleteProgram(m_id);
}

bool ComputeShader::loadFromFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "[ComputeShader] Could not open: " << path << "\n";
        return false;
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return loadFromSource(ss.str());
}

bool ComputeShader::loadFromSource(const std::string& src) {
    return compile(src);
}

bool ComputeShader::compile(const std::string& src) {
    if (m_id) { glDeleteProgram(m_id); m_id = 0; }

    GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
    const char* c = src.c_str();
    glShaderSource(shader, 1, &c, nullptr);
    glCompileShader(shader);

    GLint ok = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
        std::cerr << "[ComputeShader] Compile error:\n" << log << "\n";
        glDeleteShader(shader);
        return false;
    }

    m_id = glCreateProgram();
    glAttachShader(m_id, shader);
    glLinkProgram(m_id);
    glDeleteShader(shader);

    glGetProgramiv(m_id, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetProgramInfoLog(m_id, sizeof(log), nullptr, log);
        std::cerr << "[ComputeShader] Link error:\n" << log << "\n";
        glDeleteProgram(m_id);
        m_id = 0;
        return false;
    }
    return true;
}

void ComputeShader::use() const {
    glUseProgram(m_id);
}

void ComputeShader::setInt(const std::string& name, int v) const {
    glUniform1i(glGetUniformLocation(m_id, name.c_str()), v);
}

void ComputeShader::setFloat(const std::string& name, float v) const {
    glUniform1f(glGetUniformLocation(m_id, name.c_str()), v);
}

void ComputeShader::setFloat2(const std::string& name, float x, float y) const {
    glUniform2f(glGetUniformLocation(m_id, name.c_str()), x, y);
}

void ComputeShader::setFloat4(const std::string& name, float x, float y, float z, float w) const {
    glUniform4f(glGetUniformLocation(m_id, name.c_str()), x, y, z, w);
}

void ComputeShader::dispatch(uint32_t numGroupsX, uint32_t numGroupsY) const {
    glDispatchCompute(numGroupsX, numGroupsY, 1);
}

void ComputeShader::barrier() {
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
