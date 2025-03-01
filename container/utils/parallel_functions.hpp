#pragma once
#include <vector>
#include <array>
// #include <execution>
#include <future>
#include <memory>
#include <cstring>
#include <limits>
#include "utils/PAM/_deps/parlaylib-src/include/parlay/internal/sample_sort.h"
#include "utils/PAM/_deps/parlaylib-src/include/parlay/primitives.h"
using PUU = std::pair<uint64_t, uint64_t>;

void sort_updates(std::vector<PUU>& edges) {
    parlay::sort_inplace(edges, [] (const PUU &a, const PUU &b) {
        return (a.first != b.first) ? (a.first < b.first) : (a.second < b.second);
    });
}