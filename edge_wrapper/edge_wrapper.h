#pragma once
#include <functional>
#include "types/types.hpp"

template<class W>
void insert_edge(W &w, uint64_t src, uint64_t dest, uint64_t timestamp) {
    w.insert_edge(src, dest, timestamp);
}

template<class W>
bool has_edge(W &w, uint64_t src, uint64_t dest, uint64_t timestamp) {
    return w.has_edge(src, dest, timestamp);
}

template<typename F>
auto make_callback(F&& func) {
    return [func = std::forward<F>(func)](auto&&... args) -> bool {
        if constexpr (std::is_same_v<decltype(func(std::forward<decltype(args)>(args)...)), void>) {
            func(std::forward<decltype(args)>(args)...);
            return true; 
        } else {
            return func(std::forward<decltype(args)>(args)...);
        }
    };
}

template<class W, class F>
uint64_t edges(W &w, uint64_t src, F&& callback, uint64_t timestamp) {
    return w.edges(src, make_callback(callback), timestamp);
}

template<class W>
void init_neighbor(W &w, uint64_t src, std::vector<uint64_t> & dest, uint64_t start, uint64_t end, EdgeDriverConfig exp_cfg) {
    w.init_neighbor(src, dest, start, end, exp_cfg);
}

template<class W>
void init_real_graph(W &w, std::vector<operation> & stream) {
    w.init_real_graph(stream);
}

