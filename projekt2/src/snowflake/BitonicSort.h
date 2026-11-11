#pragma once
#include <vector>
#include <functional>
#include <omp.h>
#include <algorithm>
#include <cmath>

// BitonicSort - OpenMP parallel Bitonic sort
// Works on any std::vector with a comparison function
namespace BitonicSort {

// One step of bitonic merge
template<typename T, typename Cmp>
void bitonicMerge(std::vector<T>& arr, int lo, int cnt, bool asc, Cmp cmp) {
    if (cnt <= 1) return;
    int half = cnt / 2;
    #pragma omp parallel for schedule(static) if(cnt > 512)
    for (int i = lo; i < lo + half; ++i) {
        bool shouldSwap = asc ? cmp(arr[i + half], arr[i])
                              : cmp(arr[i], arr[i + half]);
        if (shouldSwap) std::swap(arr[i], arr[i + half]);
    }
    bitonicMerge(arr, lo,        half, asc, cmp);
    bitonicMerge(arr, lo + half, half, asc, cmp);
}

// Recursive bitonic sort
template<typename T, typename Cmp>
void bitonicSortRec(std::vector<T>& arr, int lo, int cnt, bool asc, Cmp cmp) {
    if (cnt <= 1) return;
    int half = cnt / 2;
    // parallel branches
    #pragma omp task if(cnt > 256)
    bitonicSortRec(arr, lo,        half, true,  cmp);
    #pragma omp task if(cnt > 256)
    bitonicSortRec(arr, lo + half, half, false, cmp);
    #pragma omp taskwait
    bitonicMerge(arr, lo, cnt, asc, cmp);
}

// Public interface: sorts the vector
// Handles non-power-of-two sizes safely using sentinel padding
template<typename T, typename Cmp>
void sort(std::vector<T>& arr, Cmp cmp) {
    if (arr.size() <= 1) return;

    size_t origSize = arr.size();
    size_t n = 1;
    while (n < origSize) n <<= 1;

    if (n == origSize) {
        #pragma omp parallel
        {
            #pragma omp single
            bitonicSortRec(arr, 0, (int)n, true, cmp);
        }
        return;
    }

    // Nem 2-hatvány méret esetén sentinel wrappert használunk,
    // így a padding elemek garantáltan a tömb legvégére kerülnek,
    // és az eredeti elemek nem vesznek el és nem duplikálódnak.
    struct Item {
        T val;
        bool isPad;
    };
    std::vector<Item> padded(n);
    for (size_t i = 0; i < origSize; ++i) {
        padded[i] = { std::move(arr[i]), false };
    }
    for (size_t i = origSize; i < n; ++i) {
        padded[i] = { padded[0].val, true }; // dummy érték
    }

    auto padCmp = [&](const Item& a, const Item& b) {
        if (a.isPad != b.isPad) {
            return !a.isPad; // a valós elem mindig megelőzi a padding elemet
        }
        return cmp(a.val, b.val);
    };

    #pragma omp parallel
    {
        #pragma omp single
        bitonicSortRec(padded, 0, (int)n, true, padCmp);
    }

    arr.resize(origSize);
    for (size_t i = 0; i < origSize; ++i) {
        arr[i] = std::move(padded[i].val);
    }
}

} // namespace BitonicSort
