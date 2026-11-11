#include "GLShader.h"
#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <iostream>

GLShader::~GLShader() {
    if (m_id) glDeleteProgram(m_id);
}

std::string GLShader::readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) { std::cerr << "[GLShader] Nem nyitható: " << path << "\n"; return {}; }
    std::ostringstream ss; ss << f.rdbuf(); return ss.str();
}

bool GLShader::compileShader(uint32_t& out, uint32_t type, const std::string& src) {
    out = glCreateShader(type);
    const char* c = src.c_str();
    glShaderSource(out, 1, &c, nullptr);
    glCompileShader(out);
    GLint ok = 0;
    glGetShaderiv(out, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[2048]; glGetShaderInfoLog(out, sizeof(log), nullptr, log);
        std::cerr << "[GLShader] Fordítási hiba:\n" << log << "\n";
        glDeleteShader(out); out = 0; return false;
    }
    return true;
}

bool GLShader::compileLink(uint32_t vert, uint32_t frag) {
    if (m_id) glDeleteProgram(m_id);
    m_id = glCreateProgram();
    glAttachShader(m_id, vert); glAttachShader(m_id, frag);
    glLinkProgram(m_id);
    glDeleteShader(vert); glDeleteShader(frag);
    GLint ok = 0; glGetProgramiv(m_id, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetProgramInfoLog(m_id, sizeof(log), nullptr, log);
        std::cerr << "[GLShader] Link hiba:\n" << log << "\n";
        glDeleteProgram(m_id); m_id = 0; return false;
    }
    return true;
}

bool GLShader::loadVF(const std::string& vertSrc, const std::string& fragSrc) {
    uint32_t v = 0, f = 0;
    if (!compileShader(v, GL_VERTEX_SHADER,   vertSrc)) return false;
    if (!compileShader(f, GL_FRAGMENT_SHADER, fragSrc)) { glDeleteShader(v); return false; }
    return compileLink(v, f);
}

bool GLShader::loadVFFiles(const std::string& vp, const std::string& fp) {
    return loadVF(readFile(vp), readFile(fp));
}

bool GLShader::loadCompute(const std::string& src) {
    if (m_id) { glDeleteProgram(m_id); m_id = 0; }
    uint32_t cs = 0;
    if (!compileShader(cs, GL_COMPUTE_SHADER, src)) return false;
    m_id = glCreateProgram();
    glAttachShader(m_id, cs); glLinkProgram(m_id); glDeleteShader(cs);
    GLint ok = 0; glGetProgramiv(m_id, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024]; glGetProgramInfoLog(m_id, sizeof(log), nullptr, log);
        std::cerr << "[GLShader] Compute link hiba:\n" << log << "\n";
        glDeleteProgram(m_id); m_id = 0; return false;
    }
    return true;
}

bool GLShader::loadComputeFile(const std::string& path) { return loadCompute(readFile(path)); }

void GLShader::use() const { glUseProgram(m_id); }
void GLShader::dispatch(uint32_t gx, uint32_t gy, uint32_t gz) const { glDispatchCompute(gx, gy, gz); }
void GLShader::barrier() { glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT | GL_TEXTURE_FETCH_BARRIER_BIT); }

void GLShader::setInt(const char* n, int v)    const { glUniform1i(glGetUniformLocation(m_id,n),v); }
void GLShader::setFloat(const char* n, float v) const { glUniform1f(glGetUniformLocation(m_id,n),v); }
void GLShader::setFloat2(const char* n, float x, float y) const { glUniform2f(glGetUniformLocation(m_id,n),x,y); }
void GLShader::setFloat3(const char* n, float x, float y, float z) const { glUniform3f(glGetUniformLocation(m_id,n),x,y,z); }
void GLShader::setFloat4(const char* name, float x, float y, float z, float w) const {
    glUniform4f(glGetUniformLocation(m_id, name), x, y, z, w);
}
void GLShader::setDouble(const char* name, double v) const {
    glUniform1d(glGetUniformLocation(m_id, name), v);
}
void GLShader::setDouble2(const char* name, double x, double y) const {
    glUniform2d(glGetUniformLocation(m_id, name), x, y);
}
void GLShader::setMat4(const char* name, const float* m) const {
    glUniformMatrix4fv(glGetUniformLocation(m_id, name), 1, GL_FALSE, m);
}
