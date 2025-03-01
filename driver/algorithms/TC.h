#ifndef TC_HPP
#define TC_HPP

#include <iostream>
#include <memory>
#include <string>
#include <random>
#include <vector>
#include <utility>
#include <csignal>
#include <atomic>
#include <chrono>
#include <thread>
#include <omp.h>
#include <mutex>
#include "utils/log/log.h"

template <class F, class S>
class TriangleCounting {
    F & m_method;
    S m_snapshot;

public:
    TriangleCounting(F & method, S snapshot);
    ~TriangleCounting();

    uint64_t run_tc();
};

template <class F, class S>
TriangleCounting<F, S>::TriangleCounting(F & method, S snapshot) 
    :  m_method(method), m_snapshot(snapshot) {}

template <class F, class S>
TriangleCounting<F, S>::~TriangleCounting() {}

template <class F, class S>
uint64_t TriangleCounting<F, S>::run_tc() {
    auto start = std::chrono::high_resolution_clock::now();
    
    auto num_vertices = wrapper::snapshot_vertex_count(m_snapshot);
    uint64_t num_triangles = 0;
    // std::vector<std::pair<uint64_t, uint64_t>> edges;
    for (uint64_t i = 0; i < num_vertices; i++) {
        auto get_edges = [&] (uint64_t dst, double weight) {
            if (dst > i) {
                auto res = wrapper::snapshot_intersect(m_snapshot, i, dst);
                num_triangles += res;
            }
        };
        wrapper::snapshot_edges(m_snapshot, i, get_edges, true);
    }
   
    num_triangles /= 3;

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Gapbs TC took " << duration.count() << " milliseconds" << std::endl;
    log_info("TC: %ld milliseconds", duration.count());
    std::cout << "triangle count: " << num_triangles << std::endl;
    return num_triangles;
}


#endif // TC HPP
