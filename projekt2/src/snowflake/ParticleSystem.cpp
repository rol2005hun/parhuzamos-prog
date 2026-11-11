#include "ParticleSystem.h"
#include "BitonicSort.h"
#include <glad/glad.h>
#include <omp.h>
#include <cmath>
#include <algorithm>

void ParticleSystem::init(int maxP) {
    maxParticles = maxP;
    m_particles.reserve(maxP);

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    // Layout: x, y, size, r, g, b, a (based on life)
    glBufferData(GL_ARRAY_BUFFER, maxP * 7 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    // attrib 0: pos (xy)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // attrib 1: size
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)(2*sizeof(float)));
    glEnableVertexAttribArray(1);
    // attrib 2: color rgba
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 7*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
}

void ParticleSystem::destroy() {
    if (vbo) glDeleteBuffers(1, &vbo);
    if (vao) glDeleteVertexArrays(1, &vao);
}

void ParticleSystem::spawnParticle(Vec2 pos) {
    std::uniform_real_distribution<float> rndAngle(0.0f, 6.28318f);
    std::uniform_real_distribution<float> rndSpeed(0.1f, 1.0f);
    std::uniform_real_distribution<float> rndSize(3.0f, 10.0f);
    std::uniform_real_distribution<float> rndDepth(0.0f, 1.0f);
    std::uniform_real_distribution<float> rndBlue(0.7f, 1.0f);

    float angle = rndAngle(m_rng);
    float spd   = rndSpeed(m_rng) * speed;

    Particle p;
    p.x     = pos.x;
    p.y     = pos.y;
    p.vx    = std::cos(angle) * spd;
    p.vy    = std::sin(angle) * spd + 0.1f; // slight upward initial velocity
    p.life  = 1.0f;
    p.size  = rndSize(m_rng);
    p.depth = rndDepth(m_rng);
    p.r     = 0.85f + rndBlue(m_rng) * 0.1f;
    p.g     = 0.92f + rndBlue(m_rng) * 0.05f;
    p.b     = 1.0f;
    m_particles.push_back(p);
}

void ParticleSystem::update(float dt, const std::vector<Vec2>& spawnPoints) {
    if (spawnPoints.empty()) return;

    // Spawn new particles
    m_spawnAccum += spawnRate * dt;
    int toSpawn = (int)m_spawnAccum;
    m_spawnAccum -= toSpawn;

    std::uniform_int_distribution<int> rndIdx(0, (int)spawnPoints.size()-1);
    for (int i = 0; i < toSpawn && (int)m_particles.size() < maxParticles; ++i) {
        spawnParticle(spawnPoints[rndIdx(m_rng)]);
    }

    // Update with OpenMP
    float decay = dt / lifeTime;

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < (int)m_particles.size(); ++i) {
        Particle& p = m_particles[i];
        p.vx += wind    * dt;
        p.vy += gravity * dt;
        p.x  += p.vx * dt;
        p.y  += p.vy * dt;
        p.life -= decay;
    }

    // remove dead particles
    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(),
                       [](const Particle& p){ return p.life <= 0.0f; }),
        m_particles.end());

    // bitonic sort for depth
    bitonicSort();
}

void ParticleSystem::bitonicSort() {
    // CPU Bitonic sort - with OpenMP parallelization
    BitonicSort::sort(m_particles, [](const Particle& a, const Particle& b){
        return a.depth < b.depth;
    });
}

void ParticleSystem::uploadToGPU() {
    if (m_particles.empty()) return;

    // Flat float array: [x, y, size, r, g, b, alpha]
    std::vector<float> data;
    data.reserve(m_particles.size() * 7);
    for (const auto& p : m_particles) {
        data.push_back(p.x);
        data.push_back(p.y);
        data.push_back(p.size * p.life); // size decreases at the end of life
        data.push_back(p.r);
        data.push_back(p.g);
        data.push_back(p.b);
        data.push_back(p.life * p.life); // alpha = life^2 (smooth fade out)
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    data.size() * sizeof(float), data.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}
