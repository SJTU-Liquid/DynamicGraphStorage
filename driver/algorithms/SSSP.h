#ifndef SSSP_HPP
#define SSSP_HPP

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
class ssspExperiments {
    const int m_num_threads;

    double m_delta;
    std::mutex m_mutex;

    F & m_method;
    S m_snapshot;
public:
    ssspExperiments(const int num_threads, const double delta, F & method, S snapshot);
    ~ssspExperiments();

    void run_sssp(uint64_t source, std::vector<std::pair<uint64_t, double>> & external_ids);
private:
    gapbs::pvector<double> sssp(uint64_t source);
};

template <class F, class S>
ssspExperiments<F, S>::ssspExperiments(const int num_threads, const double delta, F & method, S snapshot)
    : m_num_threads(num_threads), m_delta(delta), m_method(method), m_snapshot(snapshot) {}

template <class F, class S>
ssspExperiments<F, S>::~ssspExperiments() {}

template <class F, class S>
gapbs::pvector<double> ssspExperiments<F, S>::sssp(uint64_t source) {
    const uint64_t num_vertices = wrapper::snapshot_vertex_count(m_snapshot);
    const uint64_t num_edges = wrapper::snapshot_edge_count(m_snapshot);
    const size_t kMaxBin = std::numeric_limits<size_t>::max()/2;

    // Initialize distances to inf
    gapbs::pvector<double> dist(num_vertices, std::numeric_limits<double>::infinity());
    dist[source] = 0;
    gapbs::pvector<uint64_t> frontier(num_edges);

    size_t shared_indexes[2] = {0, kMaxBin};
    size_t frontier_tails[2] = {1, 0};

    frontier[0] = source;

    std::vector<std::vector<uint64_t>> local_bins(0);
    
    size_t iter = 0;
    while(shared_indexes[iter & 1] != kMaxBin) {
        std::vector<std::thread> threads;
        size_t &curr_bin_index = shared_indexes[iter & 1];
        size_t &next_bin_index = shared_indexes[(iter + 1) & 1];
        size_t &curr_frontier_tail = frontier_tails[iter & 1];
        size_t &next_frontier_tail = frontier_tails[(iter + 1) & 1];

        size_t chunk_size = (curr_frontier_tail + m_num_threads - 1) / m_num_threads;
        wrapper::set_max_threads(m_method, m_num_threads);
        for (int i = 0; i < m_num_threads; i++) {
            threads.emplace_back(std::thread([this, &dist, chunk_size, &curr_frontier_tail, &local_bins, &curr_bin_index, &frontier] (int thread_id) {
                wrapper::init_thread(m_method, thread_id);
                
                auto snapshot_local = wrapper::snapshot_clone(m_snapshot);

                size_t start = thread_id * chunk_size;
                size_t end = std::min((thread_id + 1) * chunk_size, curr_frontier_tail);

                for (size_t i = start; i < end; i++) {
                    uint64_t u = frontier[i];

                    if (dist[u] >= m_delta * static_cast<double>(curr_bin_index)) {
                        wrapper::snapshot_edges(snapshot_local, u, [this, &dist, &local_bins, u](uint64_t v, double w){
                            if (v >= dist.size() || u >= dist.size() || u == v) return;
                            double old_dist = dist[v];
                            double new_dist = dist[u] + 1;

                            if (new_dist < old_dist) {
                                bool changed_dist = true;
                                while(!gapbs::compare_and_swap(dist[v], old_dist, new_dist)) {
                                    old_dist = dist[v];
                                    if (new_dist >= old_dist) {
                                        changed_dist = false;
                                        break;
                                    }
                                }

                                if (changed_dist) {
                                    size_t bin_index = static_cast<size_t>(new_dist / m_delta);
                                    std::lock_guard<std::mutex> lock(m_mutex);
                                    if (bin_index >= local_bins.size()) {
                                        local_bins.resize(bin_index + 1);
                                    }
                                    local_bins[bin_index].push_back(v);
                                }
                            }

                        }, false);
                    }
                }

                wrapper::end_thread(m_method, thread_id);
            }, i));
        }

        for (auto &thread : threads) {
            thread.join();
        }


        for (size_t i = curr_bin_index; i < local_bins.size(); i++) {
            if (!local_bins[i].empty()) {
                #pragma omp critical
                next_bin_index = std::min(next_bin_index, i);
                break;
            }
        }

        curr_bin_index = kMaxBin;
        curr_frontier_tail = 0;

        if (next_bin_index < local_bins.size()) {
            size_t copy_start = gapbs::fetch_and_add(next_frontier_tail, local_bins[next_bin_index].size());
            copy(local_bins[next_bin_index].begin(), local_bins[next_bin_index].end(), frontier.data() + copy_start);
            local_bins[next_bin_index].resize(0);
        }

        iter++;
    }
    return dist;
}

template <class F, class S>
void ssspExperiments<F, S>::run_sssp(uint64_t source, std::vector<std::pair<uint64_t, double>> & external_ids) {

    source = wrapper::snapshot_logical2physical(m_snapshot, source);
    auto start = std::chrono::high_resolution_clock::now();
    auto dist = sssp(source);

    std::vector<std::thread> threads;
    
    auto num_vertices = wrapper::snapshot_vertex_count(m_snapshot);
    
    uint64_t chunk_size = (num_vertices + m_num_threads - 1) / m_num_threads;
    external_ids.resize(num_vertices);

    wrapper::set_max_threads(m_method, m_num_threads);
    for (int i = 0; i < m_num_threads; i++) {
        threads.emplace_back(std::thread([this, &dist, chunk_size, &external_ids, num_vertices] (int thread_id) {
            wrapper::init_thread(m_method, thread_id);
            uint64_t start = thread_id * chunk_size;
            uint64_t end = std::min(start + chunk_size, num_vertices);
            
            auto snapshot_local = wrapper::snapshot_clone(m_snapshot);

            for (int64_t u = start; u < end; u++) {
                external_ids[u] = std::make_pair(wrapper::snapshot_physical2logical(m_snapshot, u), dist[u]);
            }

            wrapper::end_thread(m_method, thread_id);
        }, i));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Gapbs SSSP took " << duration.count() << " milliseconds" << std::endl;
    log_info("SSSP: %ld milliseconds", duration.count());
    
}

#endif