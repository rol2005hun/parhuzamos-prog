#include "KochGenerator.h"
#include <omp.h>
#include <cmath>
#include <algorithm>

static constexpr float PI = 3.14159265f;

Vec2 KochGenerator::rotate(Vec2 v, float angleDeg) {
    float rad = angleDeg * PI / 180.0f;
    float c = std::cos(rad), s = std::sin(rad);
    return { v.x * c - v.y * s, v.x * s + v.y * c };
}

void KochGenerator::subdivide(Vec2 a, Vec2 b, int d, int index, int stride) {
    if (d == 0) {
        m_verts[index] = a;
        return;
    }
    // subdivide segment: p1 = a + ratio*(b-a), p2 = a + (1-ratio)*(b-a)
    float r1 = ratio, r2 = 1.0f - ratio;
    Vec2 p1 = { a.x + r1 * (b.x - a.x), a.y + r1 * (b.y - a.y) };
    Vec2 p2 = { a.x + r2 * (b.x - a.x), a.y + r2 * (b.y - a.y) };

    // Calculate peak point - rotated vector between p1 and p2
    Vec2 mid = { (p1.x + p2.x) * 0.5f, (p1.y + p2.y) * 0.5f };
    Vec2 dir = { p2.x - p1.x, p2.y - p1.y };
    float a2 = asymmetric ? angle * 0.8f : angle;
    Vec2 peak = rotate(dir, -a2);
    Vec2 tip  = { p1.x + peak.x * 0.5f, p1.y + peak.y * 0.5f };
    // Adjust peak around the p1-p2 midpoint
    float len = std::sqrt(dir.x*dir.x + dir.y*dir.y);
    Vec2 perp = { -dir.y / len, dir.x / len };
    float h = std::tan(a2 * PI / 180.0f) * len * 0.5f;
    tip = { mid.x + perp.x * h, mid.y + perp.y * h };

    int next_stride = stride / 4;

    // Parallel branches with OpenMP tasks if we are deep enough
    if (d > 4) {
        #pragma omp task
        subdivide(a, p1, d - 1, index, next_stride);
        #pragma omp task
        subdivide(p1, tip, d - 1, index + next_stride, next_stride);
        #pragma omp task
        subdivide(tip, p2, d - 1, index + 2 * next_stride, next_stride);
        #pragma omp task
        subdivide(p2, b, d - 1, index + 3 * next_stride, next_stride);
        #pragma omp taskwait
    } else {
        subdivide(a, p1, d - 1, index, next_stride);
        subdivide(p1, tip, d - 1, index + next_stride, next_stride);
        subdivide(tip, p2, d - 1, index + 2 * next_stride, next_stride);
        subdivide(p2, b, d - 1, index + 3 * next_stride, next_stride);
    }
}

void KochGenerator::generate() {
    // 3 equilateral triangle corners
    float r = scale;
    Vec2 corners[3] = {
        { r * std::cos(PI/2.0f + 0*2.0f*PI/3.0f),
          r * std::sin(PI/2.0f + 0*2.0f*PI/3.0f) },
        { r * std::cos(PI/2.0f + 1*2.0f*PI/3.0f),
          r * std::sin(PI/2.0f + 1*2.0f*PI/3.0f) },
        { r * std::cos(PI/2.0f + 2*2.0f*PI/3.0f),
          r * std::sin(PI/2.0f + 2*2.0f*PI/3.0f) },
    };

    // Pre-allocate memory to avoid thread contention
    int num_segments_per_side = 1 << (depth * 2); // 4^depth
    int total_segments = 3 * num_segments_per_side;
    m_verts.resize(total_segments + 1);

    // Distribute OpenMP tasks among all processor cores
    #pragma omp parallel
    {
        #pragma omp single nowait
        {
            for (int i = 0; i < 3; ++i) {
                Vec2 a = corners[i];
                Vec2 b = corners[(i + 1) % 3];
                #pragma omp task
                subdivide(a, b, depth, i * num_segments_per_side, num_segments_per_side);
            }
        }
    }
    
    // Closing point
    m_verts[total_segments] = corners[0];

    // Collect vertices (every depth^4-th point is a snowflake tip)
    m_tips.clear();
    for (size_t i = 0; i < m_verts.size(); i += std::max(1, (int)m_verts.size() / 80)) {
        m_tips.push_back(m_verts[i]);
    }
}
