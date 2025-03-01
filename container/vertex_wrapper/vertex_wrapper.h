#pragma once
#include <functional>
#include "types/types.hpp"

template<class W>
void insert_vertex(W &w, uint64_t src) {
    w.insert_vertex(src, nullptr, 0);
}

template<class W>
bool has_vertex(W &w, uint64_t src) {
    return w.has_vertex(src);
}
