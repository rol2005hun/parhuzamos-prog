#pragma once
#include <string>
#include <cstdint>

// ComputeShader - OpenGL 4.3+ compute shader wrapper
// Manages shader loading, compilation, dispatch
class ComputeShader {
public:
    ComputeShader() = default;
    ~ComputeShader();

    // Load shader from file (.comp)
    bool loadFromFile(const std::string& path);
    // Load shader from string source
    bool loadFromSource(const std::string& src);

    void use() const;

    // Uniform setters
    void setInt(const std::string& name, int v) const;
    void setFloat(const std::string& name, float v) const;
    void setFloat2(const std::string& name, float x, float y) const;
    void setFloat4(const std::string& name, float x, float y, float z, float w) const;

    // Dispatch: numGroupsX * numGroupsY * 1 workgroup
    void dispatch(uint32_t numGroupsX, uint32_t numGroupsY) const;

    // GPU-CPU sync
    static void barrier();

    uint32_t id() const { return m_id; }
    bool valid() const  { return m_id != 0; }

private:
    uint32_t m_id = 0;
    bool compile(const std::string& src);
};
