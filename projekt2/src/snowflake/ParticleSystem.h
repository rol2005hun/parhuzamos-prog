#pragma once
#include "KochGenerator.h"
#include <vector>
#include <random>
#include <cstdint>

// Particle - a single snow particle
struct Particle {
    float x, y;       // position
    float vx, vy;     // velocity
    float life;       // lifespan [0,1] (1=new, 0=dead)
    float size;       // size
    float depth;      // depth (key for Bitonic sort)
    float r, g, b;    // color
};

// ParticleSystem - animated snow particle system
class ParticleSystem {
public:
    int   maxParticles = 2000;
    float gravity      = -0.05f;  // downward pull
    float wind         = 0.02f;   // horizontal drift
    float spawnRate    = 50.0f;   // particles/sec
    float lifeTime     = 3.0f;    // in seconds
    float speed        = 0.3f;    // initial velocity scale

    // GPU VBO ID (externally bindable)
    uint32_t vbo = 0;
    uint32_t vao = 0;

    void init(int maxP = 2000);
    void destroy();

    // dt: delta time (sec)
    // spawnPoints: spawns particles from the vertices of the snowflake
    void update(float dt, const std::vector<Vec2>& spawnPoints);

    // Bitonic sort by depth (for GPU sorting rendering order)
    void bitonicSort();

    // upload vbo
    void uploadToGPU();

    int activeCount() const { return (int)m_particles.size(); }

private:
    std::vector<Particle> m_particles;
    std::mt19937          m_rng{ 42 };
    float                 m_spawnAccum = 0.0f;

    void spawnParticle(Vec2 pos);
};
