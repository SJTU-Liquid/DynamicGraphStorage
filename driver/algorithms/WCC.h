#ifndef WCC_HPP
#define WCC_HPP

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
class wccExperiments {
    const int m_num_threads;

    F & m_method;
    S m_snapshot;

public:
    wccExperiments(const int num_threads, F & method, S snapshot);
    ~wccExperiments();

    void run_wcc(std::vector<std::pair<uint64_t, int64_t>> & external_ids);

private:
    std::unique_ptr<uint64_t[]> wcc();
    
};

template <class F, class S>
wccExperiments<F, S>::wccExperiments(const int num_threads, F & method, S snapshot) 
    : m_num_threads(num_threads), m_method(method), m_snapshot(snapshot) {}

template <class F, class S>
wccExperiments<F, S>::~wccExperiments() {}

template <class F, class S>
std::unique_ptr<uint64_t[]> wccExperiments<F, S>::wcc() {
    const uint64_t num_vertices = wrapper::snapshot_vertex_count(m_snapshot);
    std::unique_ptr<uint64_t[]> ptr_components { new uint64_t[num_vertices] };
    uint64_t * comp = ptr_components.get();

    #pragma omp parallel for
    for (uint64_t i = 0; i < num_vertices; i++) {
        comp[i] = i;
    }

    bool change = true;
    std::vector<std::thread> threads;
    uint64_t chunk_size = (num_vertices + m_num_threads - 1) / m_num_threads;
    while(change) {
        change = false;
        
        for (int i = 0; i < m_num_threads; i++) {
            threads.emplace_back(std::thread([this, &change, &comp, num_vertices, chunk_size](int thread_id) {
                wrapper::init_thread(m_method, thread_id);
                auto snapshot_local = wrapper::snapshot_clone(m_snapshot);
                uint64_t start = chunk_size * thread_id;
                uint64_t end = std::min(start + chunk_size, num_vertices);
                
                for (uint64_t u = start; u < end; u++) {
                    wrapper::snapshot_edges(snapshot_local, u, [comp, &change, u, num_vertices](uint64_t v, double w) {
                        uint64_t comp_u = comp[u];
                        uint64_t comp_v = comp[v];
                        if (comp_u == comp_v) {
                            return;
                        }

                        uint64_t high_comp = std::max(comp_u, comp_v);
                        uint64_t low_comp = std::min(comp_u, comp_v);
                        
                        if (high_comp >= num_vertices || low_comp >= num_vertices) return;
                        if (high_comp == comp[high_comp]) {
                            change = true;
                            comp[high_comp] = low_comp;
                        }

                        return;
                    }, false);
                }

                wrapper::end_thread(m_method, thread_id);
                
            }, i));
        }

        for (auto & thread : threads) {
            thread.join();
        }

        threads.clear();
        
        #pragma omp parallel for
        for (uint64_t i = 0; i < num_vertices; i++) {
            while(comp[i] != comp[comp[i]]) {
                comp[i] = comp[comp[i]];
            }
        }
    }

    return ptr_components;
}

template <class F, class S>
void wccExperiments<F, S>::run_wcc(std::vector<std::pair<uint64_t, int64_t>> & external_ids) {
    auto start = std::chrono::high_resolution_clock::now();

    std::unique_ptr<uint64_t[]> ptr_components = wcc();
    
    std::vector<std::thread> threads;
    auto num_vertices = wrapper::snapshot_vertex_count(m_snapshot);
    
    uint64_t chunk_size = (num_vertices + m_num_threads - 1) / m_num_threads;
    external_ids.resize(num_vertices);

    wrapper::set_max_threads(m_method, m_num_threads);
    for (int i = 0; i < m_num_threads; i++) {
        threads.emplace_back(std::thread([this, &ptr_components, chunk_size, &external_ids, num_vertices] (int thread_id) {
            wrapper::init_thread(m_method, thread_id);
            uint64_t start = thread_id * chunk_size;
            uint64_t end = std::min(start + chunk_size, num_vertices);
            
            auto snapshot_local = wrapper::snapshot_clone(m_snapshot);

            for (int64_t u = start; u < end; u++) {
                external_ids[u] = std::make_pair(wrapper::snapshot_physical2logical(snapshot_local, u), wrapper::snapshot_physical2logical(m_snapshot, ptr_components[u]));
            }

            wrapper::end_thread(m_method, thread_id);
        }, i));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Gapbs WCC took " << duration.count() << " milliseconds" << std::endl;
    log_info("WCC: %ld milliseconds", duration.count());
}


#endif // WCC_HPP
