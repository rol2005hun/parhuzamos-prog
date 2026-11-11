#pragma once
#include <array>
#include <string>
#include <vector>

struct HistogramResult {
    std::array<int,256> r{}, g{}, b{};
    double timeMs = 0.0;
    std::string label;
};

namespace Histogram {
    HistogramResult computeSingle      (const float* rgba, int pixelCount);
    HistogramResult computeMultiAtomic1(const float* rgba, int pixelCount);
    HistogramResult computeMultiAtomic3(const float* rgba, int pixelCount);
    HistogramResult computeMultiLocal  (const float* rgba, int pixelCount);
}
