#pragma once
#include <vector>
#include <array>

struct Vec2 { float x, y; };

// KochGenerator - parallel Koch snowflake generator
// output: list of vertices (line strip segments)
class KochGenerator {
public:
    // parameters (adjustable from UI)
    int   depth     = 4;       // recursion depth (1-10)
    float angle     = 60.0f;   // inner angle in degrees (30-90)
    float ratio     = 1.0f/3.0f; // segment ratio (0.2-0.5)
    bool  asymmetric = false;  // asymmetric variation
    float scale     = 0.7f;    // size of the snowflake in [-1,1] space

    // Generates the snowflake vertices (3 branches)
    // using OpenMP tasks
    void generate();

    // vertices for vbo
    const std::vector<Vec2>& vertices() const { return m_verts; }

    // Positions of the snowflake vertices (for particle spawning)
    const std::vector<Vec2>& tips() const { return m_tips; }

private:
    std::vector<Vec2> m_verts;
    std::vector<Vec2> m_tips;

    // One segment Koch recursion
    void subdivide(Vec2 a, Vec2 b, int d, int index, int stride);
    Vec2 rotate(Vec2 v, float angleDeg);
};
