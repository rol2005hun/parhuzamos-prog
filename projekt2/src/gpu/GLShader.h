#pragma once
#include <string>
#include <cstdint>

// GLShader — vertex+fragment VAGY compute shader program
class GLShader {
public:
    GLShader() = default;
    ~GLShader();

    // Vertex + Fragment shader forrásból
    bool loadVF(const std::string& vertSrc, const std::string& fragSrc);
    bool loadVFFiles(const std::string& vertPath, const std::string& fragPath);

    // Compute shader
    bool loadCompute(const std::string& src);
    bool loadComputeFile(const std::string& path);

    void use() const;
    void dispatch(uint32_t gx, uint32_t gy, uint32_t gz = 1) const;
    static void barrier();

    void setInt(const char* name, int v)   const;
    void setFloat(const char* name, float v) const;
    void setFloat2(const char* name, float x, float y) const;
    void setFloat3(const char* name, float x, float y, float z) const;
    
    void setDouble(const char* name, double v) const;
    void setDouble2(const char* name, double x, double y) const;
    void setFloat4(const char* name, float x, float y, float z, float w) const;
    void setMat4(const char* name, const float* m) const;

    bool valid() const { return m_id != 0; }
    uint32_t id() const { return m_id; }

private:
    uint32_t m_id = 0;
    bool compileLink(uint32_t vert, uint32_t frag);
    bool compileShader(uint32_t& out, uint32_t type, const std::string& src);
    static std::string readFile(const std::string& path);
};
