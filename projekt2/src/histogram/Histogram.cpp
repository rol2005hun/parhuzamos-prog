#include "Histogram.h"
#include <omp.h>
#include <atomic>
#include <cstring>
#include <chrono>
#include <cmath>
#include <algorithm>

// single thread
HistogramResult Histogram::computeSingle(const float* rgba, int pixelCount) {
    HistogramResult res;
    res.label = "Single Thread";
    auto t0 = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < pixelCount; ++i) {
        int r = std::min((int)(rgba[i*4+0] * 255), 255);
        int g = std::min((int)(rgba[i*4+1] * 255), 255);
        int b = std::min((int)(rgba[i*4+2] * 255), 255);
        res.r[r]++; res.g[g]++; res.b[b]++;
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    res.timeMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return res;
}

// multi-thread with 1 shared atomic array
HistogramResult Histogram::computeMultiAtomic1(const float* rgba, int pixelCount) {
    HistogramResult res;
    res.label = "Multi: 1 atomic[256×4]";
    auto t0 = std::chrono::high_resolution_clock::now();

    // 256*3 atomic int: [R0..R255, G0..G255, B0..B255]
    static std::atomic<int> hist[256 * 3];
    for (auto& a : hist) a.store(0, std::memory_order_relaxed);

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < pixelCount; ++i) {
        int r = std::min((int)(rgba[i*4+0] * 255), 255);
        int g = std::min((int)(rgba[i*4+1] * 255), 255);
        int b = std::min((int)(rgba[i*4+2] * 255), 255);
        hist[r          ].fetch_add(1, std::memory_order_relaxed);
        hist[256 + g    ].fetch_add(1, std::memory_order_relaxed);
        hist[512 + b    ].fetch_add(1, std::memory_order_relaxed);
    }

    for (int i = 0; i < 256; ++i) {
        res.r[i] = hist[i].load();
        res.g[i] = hist[256+i].load();
        res.b[i] = hist[512+i].load();
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    res.timeMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return res;
}

// multi-thread with 3 atomic arrays
HistogramResult Histogram::computeMultiAtomic3(const float* rgba, int pixelCount) {
    HistogramResult res;
    res.label = "Multi: 3 külön atomic[]";
    auto t0 = std::chrono::high_resolution_clock::now();

    static std::atomic<int> histR[256], histG[256], histB[256];
    for (auto& a : histR) a.store(0, std::memory_order_relaxed);
    for (auto& a : histG) a.store(0, std::memory_order_relaxed);
    for (auto& a : histB) a.store(0, std::memory_order_relaxed);

    #pragma omp parallel for schedule(static)
    for (int i = 0; i < pixelCount; ++i) {
        int r = std::min((int)(rgba[i*4+0] * 255), 255);
        int g = std::min((int)(rgba[i*4+1] * 255), 255);
        int b = std::min((int)(rgba[i*4+2] * 255), 255);
        histR[r].fetch_add(1, std::memory_order_relaxed);
        histG[g].fetch_add(1, std::memory_order_relaxed);
        histB[b].fetch_add(1, std::memory_order_relaxed);
    }

    for (int i = 0; i < 256; ++i) {
        res.r[i] = histR[i].load();
        res.g[i] = histG[i].load();
        res.b[i] = histB[i].load();
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    res.timeMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return res;
}

// multi-thread local arrays
HistogramResult Histogram::computeMultiLocal(const float* rgba, int pixelCount) {
    HistogramResult res;
    res.label = "Multi: lokális + összegzés";
    auto t0 = std::chrono::high_resolution_clock::now();

    int nThreads = omp_get_max_threads();
    // thread local arrays to avoid race condition
    std::vector<int> localR(nThreads * 256, 0);
    std::vector<int> localG(nThreads * 256, 0);
    std::vector<int> localB(nThreads * 256, 0);

    #pragma omp parallel
    {
        int tid = omp_get_thread_num();
        int* lr = localR.data() + tid * 256;
        int* lg = localG.data() + tid * 256;
        int* lb = localB.data() + tid * 256;

        #pragma omp for schedule(static)
        for (int i = 0; i < pixelCount; ++i) {
            lr[std::min((int)(rgba[i*4+0]*255), 255)]++;
            lg[std::min((int)(rgba[i*4+1]*255), 255)]++;
            lb[std::min((int)(rgba[i*4+2]*255), 255)]++;
        }
    }

    // sum up
    for (int t = 0; t < nThreads; ++t) {
        for (int i = 0; i < 256; ++i) {
            res.r[i] += localR[t*256 + i];
            res.g[i] += localG[t*256 + i];
            res.b[i] += localB[t*256 + i];
        }
    }

    auto t1 = std::chrono::high_resolution_clock::now();
    res.timeMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return res;
}
