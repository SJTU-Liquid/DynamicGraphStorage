#pragma once

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
    for (uint64_t src = 0; src < num_vertices; src++) {
        auto iter_outer = wrapper::snapshot_begin(m_snapshot, src);
        while (iter_outer.is_valid()) {
            auto dest = *iter_outer;
            if (dest > src) break;
            auto iter_src = wrapper::snapshot_begin(m_snapshot, src);
            auto iter_dest = wrapper::snapshot_begin(m_snapshot, dest);
            
            while (iter_src.is_valid() && iter_dest.is_valid()) {
                if (*iter_dest > dest) break;

                if (*iter_dest > *iter_src) {
                    do {
                        ++iter_src;
                    } while (iter_src.is_valid() && *iter_dest > *iter_src);
                }
                
                else if (*iter_dest == *iter_src) {
                    num_triangles += 1;
                    ++iter_src;
                    ++iter_dest;
                }

                else {
                    do {
                        ++iter_dest;
                    } while (iter_dest.is_valid() && *iter_dest < *iter_src && *iter_dest < dest);
                }
            }
            ++iter_outer;
        }
    }
   

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Gapbs TC took " << duration.count() << " milliseconds" << std::endl;
    log_info("TC: %ld milliseconds", duration.count());
    std::cout << "triangle count: " << num_triangles << std::endl;
    return num_triangles;
}

