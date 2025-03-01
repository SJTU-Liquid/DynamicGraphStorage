#ifndef PAGE_RANK_HPP
#define PAGE_RANK_HPP

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

template <class F, class S>
class pageRankExperiments {
    const int m_num_threads;
    const uint64_t m_num_iterations;
    const double m_damping_factor;
    
    F & m_method;
    S m_snapshot;

public:
    pageRankExperiments(const int num_threads, const uint64_t num_iterations, const double damping_factor, F & method, S snapshot);
    ~pageRankExperiments();

    void run_page_rank(std::vector<std::pair<uint64_t, double>> & external_ids);
private:
    std::unique_ptr<double[]> page_rank();
};

template <class F, class S>
pageRankExperiments<F, S>::pageRankExperiments(const int num_threads, const uint64_t num_iterations, const double damping_factor, F & method, S snapshot) 
    : m_num_threads(num_threads), m_num_iterations(num_iterations), m_damping_factor(damping_factor), m_method(method), m_snapshot(snapshot) {
        omp_set_num_threads(num_threads);
    }

template <class F, class S>
pageRankExperiments<F, S>::~pageRankExperiments() {}

template <class F, class S>
std::unique_ptr<double[]> pageRankExperiments<F, S>::page_rank() {
    const uint64_t num_vertices = wrapper::snapshot_vertex_count(m_snapshot);

    const double init_score = 1.0 / num_vertices;
    const double base_score = (1.0 - m_damping_factor) / num_vertices;

    std::unique_ptr<double[]> ptr_scores {new double[num_vertices]()};
    double * scores = ptr_scores.get();
    #pragma omp parallel for
    for (uint64_t v = 0; v < num_vertices; v++) {
        scores[v] = init_score;
    }
    gapbs::pvector<double> outgoing_contrib(num_vertices, 0.0);

    for (uint64_t iter = 0; iter < m_num_iterations; iter++) {
        std::vector<double> dangling_sums(m_num_threads, 0.0);
        double dangling_sum = 0.0;

        uint64_t chunk_size = (num_vertices + m_num_threads - 1) / m_num_threads;
        std::vector<std::thread> threads;
        try {
            wrapper::set_max_threads(m_method, m_num_threads);
            for (int i = 0; i < m_num_threads; i++) {
                threads.emplace_back(std::thread([this, &dangling_sums, &outgoing_contrib, chunk_size, num_vertices, &scores] (int thread_id) {
                    wrapper::init_thread(m_method,  thread_id);
                    auto snapshot_local = wrapper::snapshot_clone(m_snapshot);
                    uint64_t start = thread_id * chunk_size;
                    uint64_t end = std::min(start + chunk_size, num_vertices);

                    for (uint64_t v = start; v < end; v++) {
                        uint64_t out_degree = wrapper::snapshot_degree(snapshot_local, v, false);
                        if (out_degree == 0) {
                            dangling_sums[thread_id] += scores[v];
                        } else {
                            outgoing_contrib[v] = scores[v] / out_degree;
                        }
                    }
                    
                    wrapper::end_thread(m_method, thread_id);
                }, i));
            }

            for (auto &t : threads) {
                t.join();
            }

            threads.clear();
            
            for (int i = 0; i < m_num_threads; i++) {
                dangling_sum += dangling_sums[i];
            }

            dangling_sum /= num_vertices;

            for (int i = 0; i < m_num_threads; i++) {
                threads.emplace_back(std::thread([this, &dangling_sum, &outgoing_contrib, chunk_size, num_vertices, &scores, &base_score] (int thread_id) {
                    wrapper::init_thread(m_method,  thread_id);
                    auto snapshot_local = wrapper::snapshot_clone(m_snapshot);
                    uint64_t start = thread_id * chunk_size;
                    uint64_t end = std::min(start + chunk_size, num_vertices);
                    
                    for (uint64_t v = start; v < end; v++) {
                        double incoming_total = 0.0;
                        wrapper::snapshot_edges(snapshot_local, v, [&](uint64_t src, double w) {
                            if (src == v) return;
                            incoming_total += outgoing_contrib[src];
                        }, false);
                        
                        scores[v] = base_score + m_damping_factor * (incoming_total + dangling_sum);
                    }

                    wrapper::end_thread(m_method, thread_id);
                }, i));
            }

            for (auto &t : threads) {
                t.join();
            }
        } catch (std::exception &e) {
            std::cerr << "Exception: " << e.what() << std::endl;
            throw e;
        }

    }
    return ptr_scores;
}

template <class F, class S>
void pageRankExperiments<F, S>::run_page_rank(std::vector<std::pair<uint64_t, double>> & external_ids) {
    auto start = std::chrono::high_resolution_clock::now();
    auto scores = page_rank();

    std::vector<std::thread> threads;
    
    auto num_vertices = wrapper::snapshot_vertex_count(m_snapshot);
    
    uint64_t chunk_size = (num_vertices + m_num_threads - 1) / m_num_threads;
    external_ids.resize(num_vertices);

    wrapper::set_max_threads(m_method, m_num_threads);
    for (int i = 0; i < m_num_threads; i++) {
        threads.emplace_back(std::thread([this, &scores, chunk_size, &external_ids, num_vertices] (int thread_id) {
            wrapper::init_thread(m_method,  thread_id);
            uint64_t start = thread_id * chunk_size;
            uint64_t end = std::min(start + chunk_size, num_vertices);
            
            auto snapshot_local = wrapper::snapshot_clone(m_snapshot);

            for (int64_t u = start; u < end; u++) {
                // external_ids[u] = std::make_pair(wrapper::snapshot_physical2logical(snapshot_local, u), scores[u]);
                external_ids[u] = std::make_pair(u, scores[u]);
                // std::cout << u << ' ' << scores[u] << std::endl;
            }

            wrapper::end_thread(m_method, thread_id);
        }, i));
    }

    for (auto& thread : threads) {
        thread.join();
    }


    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Gapbs Page Rank took " << duration.count() << " milliseconds" << std::endl;
    log_info("PR: %ld milliseconds", duration.count());

}

#endif // PAGE_RANK_HPP
