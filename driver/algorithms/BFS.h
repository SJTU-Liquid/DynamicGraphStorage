#ifndef BFS_HPP
#define BFS_HPP

#include <iostream>
#include "third-party/gapbs.h"
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
class bfsExperiments {
    const int m_num_threads;
    const int m_alpha;
    const int m_beta;
    std::mutex m_mutex;
    F & m_method;
    S m_snapshot;

public:
    bfsExperiments(const int num_threads, const int alpha, const int beta, F& method, S snapshot);
    ~bfsExperiments();
    
    void run_gapbs_bfs(uint64_t src, std::vector<std::pair<uint64_t , int64_t>> & external_ids);
    
private:
    gapbs::pvector<std::atomic<int64_t>> init_distances();
    gapbs::pvector<std::atomic<int64_t>> bfs(uint64_t source);
    int64_t BUStep(gapbs::pvector<std::atomic<int64_t>>& distances, int64_t distance, gapbs::Bitmap &front, gapbs::Bitmap &next);
    int64_t TDStep(gapbs::pvector<std::atomic<int64_t>>& distances, int64_t distance, gapbs::SlidingQueue<int64_t>& queue);
    void BitmapToQueue(const int64_t size, const gapbs::Bitmap &bm, gapbs::SlidingQueue<int64_t> &queue);
    void QueueToBitmap(const gapbs::SlidingQueue<int64_t> &queue, gapbs::Bitmap &bm);
};

template <class F, class S>
bfsExperiments<F, S>::bfsExperiments(const int num_threads, const int alpha, const int beta, F & method, S snapshot)
        : m_num_threads(num_threads), m_alpha(alpha), m_beta(beta), m_method(method), m_snapshot(snapshot) {
            omp_set_num_threads(num_threads);
        }

template <class F, class S>
bfsExperiments<F, S>::~bfsExperiments() {}

template <class F, class S>
gapbs::pvector<std::atomic<int64_t>> bfsExperiments<F, S>::init_distances() {
    const uint64_t N = wrapper::snapshot_vertex_count(m_snapshot);
    gapbs::pvector<std::atomic<int64_t>> distances(N);

    std::atomic<uint64_t> vertex_checked = 0;
    std::vector<std::thread> threads;
    uint64_t size = N;
    uint64_t chunk_size = (size + m_num_threads - 1) / m_num_threads;
    try {
        wrapper::set_max_threads(m_method, m_num_threads);
        for (int i = 0; i < m_num_threads; i++) {
            threads.emplace_back(std::thread([this, &vertex_checked, &distances, size, chunk_size] (int thread_id) {
                wrapper::init_thread(m_method, thread_id);
                auto snapshot_local = wrapper::snapshot_clone(m_snapshot);
                uint64_t start = thread_id * chunk_size;
                uint64_t end = start + chunk_size;
                if (end > size) end = size;

                for (uint64_t i = start; i < end; i++) {
                    uint64_t out_degree = wrapper::snapshot_degree(snapshot_local, i, false);
                    distances[i] = out_degree != 0 ? -out_degree : -1;
                }

                wrapper::end_thread(m_method, thread_id);
            }, i));
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Exception in init_distances: " << e.what() << std::endl;
        throw;
    }

    for (auto& thread : threads) {
        thread.join();
    }

    return distances;
}


template <class F, class S>
void bfsExperiments<F, S>::QueueToBitmap(const gapbs::SlidingQueue<int64_t> &queue, gapbs::Bitmap &bm) {
    #pragma omp parallel for
    for (auto q_iter = queue.begin(); q_iter < queue.end(); q_iter++) {
        int64_t u = *q_iter;
        bm.set_bit_atomic(u);
    }
}

template <class F, class S>
void bfsExperiments<F, S>::BitmapToQueue(const int64_t size, const gapbs::Bitmap &bm, gapbs::SlidingQueue<int64_t> &queue) {
    const int64_t N = size;

    #pragma omp parallel
    {
        gapbs::QueueBuffer<int64_t> lqueue(queue);
        #pragma omp for
        for (int64_t n = 0; n < N; n++) {
            if (bm.get_bit(n)) lqueue.push_back(n);
        }
        lqueue.flush();
    }
    queue.slide_window();
}

template <class F, class S>
int64_t bfsExperiments<F, S>::TDStep(gapbs::pvector<std::atomic<int64_t>>& distances, int64_t distance, gapbs::SlidingQueue<int64_t>& queue) {
    std::vector<int64_t> results(m_num_threads, 0);
    uint64_t scout_count = 0;
    std::vector<std::thread> threads;

    uint64_t chunk_size = (queue.size() + m_num_threads - 1) / m_num_threads;

    try {
        wrapper::set_max_threads(m_method, m_num_threads);

        for (uint64_t i = 0; i < m_num_threads; i++) {
            threads.emplace_back(std::thread([this, &distances, distance, &scout_count, &queue, chunk_size, &results] (int thread_id) {
                wrapper::init_thread(m_method, thread_id);
                uint64_t start = thread_id * chunk_size;
                uint64_t end = start + chunk_size;
                
                if (end > queue.size()) end = queue.size();
                
                if (start < end) {
                    gapbs::QueueBuffer<int64_t> lqueue(queue);
                    
                    auto snapshot_local = wrapper::snapshot_clone(m_snapshot);
                    
                    for (auto q_iter = queue.begin() + start; q_iter != queue.end(); q_iter++) {
                        int64_t u = *q_iter;
                        
                        wrapper::snapshot_edges(snapshot_local, u, [u, &distances, distance, &lqueue, &results, thread_id](uint64_t destination, double w){
                            if (destination < distances.size() && u != destination) {
                                int64_t curr_val = distances[destination];

                                if (curr_val < 0 && distances[destination].compare_exchange_strong(curr_val, distance)) {
                                    lqueue.push_back(destination);
                                    results[thread_id] += -curr_val;
                                }
                            }
                        }, false);
                    }

                    // std::unique_lock<std::mutex> lock(m_mutex);
                    lqueue.flush();
                }
                // std::unique_lock<std::mutex> unlock(m_mutex);
                
                wrapper::end_thread(m_method, thread_id);

            }, i));
        }

        for (auto& thread : threads) {
            thread.join();
        }

        scout_count = std::accumulate(results.begin(), results.end(), 0);

    } catch (const std::exception& e) {
        std::cerr << "Exception in TDStep: " << e.what() << std::endl;
        throw e;
    }

    return scout_count;
}

template <class F, class S>
int64_t bfsExperiments<F, S>::BUStep(gapbs::pvector<std::atomic<int64_t>>& distances, int64_t distance, gapbs::Bitmap &front, gapbs::Bitmap &next) {
    const uint64_t N = m_snapshot->vertex_count();
    int64_t awake_count = 0;
    std::vector<uint64_t> results(m_num_threads, 0);
    std::vector<std::thread> threads;
    next.reset();

    uint64_t chunk_size = (N + m_num_threads - 1) / m_num_threads;

    try {
        wrapper::set_max_threads(m_method, m_num_threads);
        
        for (int i = 0; i < m_num_threads; i++) {
            threads.emplace_back(std::thread([this, &distances, distance, &front, &next, chunk_size, &awake_count, &results, N] (int thread_id) {
                wrapper::init_thread(m_method, thread_id);
                uint64_t start = thread_id * chunk_size;
                uint64_t end = std::min(start + chunk_size, N);
                auto snapshot_local = wrapper::snapshot_clone(m_snapshot);
                for (uint64_t u = start; u < end; u++) {
                    if (distances [u] < 0) {
                        bool done = false;
                        
                        wrapper::snapshot_edges(snapshot_local, u, [u, &distances, distance, &front, &next, &done, &results, &thread_id](uint64_t destination, double w){
                            if (destination < distances.size()) {
                                if (front.get_bit(destination)) {
                                    distances[u] = distance;
                                    results[thread_id]++;
                                    next.set_bit(u);
                                    done = true;
                                }
                            }
                        }, false);
                    }
                }

                wrapper::end_thread(m_method, thread_id);
            }, i));
        }

        for (auto& thread : threads) {
            thread.join();
        }

        awake_count = std::accumulate(results.begin(), results.end(), 0);

    } catch (const std::exception& e) {
        std::cerr << "Exception in BUStep: " << e.what() << std::endl;
        throw e;
    }
    return awake_count;
}

template <class F, class S>
gapbs::pvector<std::atomic<int64_t>> bfsExperiments<F, S>::bfs(uint64_t source) {
    gapbs::pvector<std::atomic<int64_t>> distances = init_distances();
    distances[source] = 0;

    gapbs::SlidingQueue<int64_t> queue(m_snapshot->vertex_count());
    queue.push_back(source);
    queue.slide_window();

    uint64_t N = wrapper::snapshot_vertex_count(m_snapshot);
    gapbs::Bitmap curr(N);
    curr.reset();
    gapbs::Bitmap front(N);
    front.reset();

    int64_t edges_to_check = wrapper::snapshot_edge_count(m_snapshot);
    int64_t scout_count = wrapper::snapshot_degree(m_snapshot, source, false);
    int64_t distance = 1;

    while(!queue.empty()) {
        if (scout_count > edges_to_check / m_alpha) {
            int64_t awake_count, old_awake_count;
            QueueToBitmap(queue, front);
            awake_count = queue.size();
            queue.slide_window();

            do {
                old_awake_count = awake_count;
                awake_count = BUStep(distances, distance, front, curr);
                front.swap(curr);
                distance++;
            } while ((awake_count >= old_awake_count) || (awake_count > m_snapshot->vertex_count() / m_beta));
            BitmapToQueue(N, front, queue);
            scout_count = 1;

        } else {
            edges_to_check -= scout_count;
            scout_count = TDStep(distances, distance, queue);
            queue.slide_window();
            distance++;
        }
    }
    
    return distances;
}

template <class F, class S>
void bfsExperiments<F, S>::run_gapbs_bfs(uint64_t source, std::vector<std::pair<uint64_t , int64_t>> & external_ids) {
    uint64_t physical_source = wrapper::snapshot_logical2physical(m_snapshot, source);
    auto start = std::chrono::high_resolution_clock::now();

    auto distances = bfs(physical_source);

    std::vector<std::thread> threads;
    uint64_t N = wrapper::snapshot_vertex_count(m_snapshot);
    uint64_t chunk_size = (N + m_num_threads - 1) / m_num_threads;
    external_ids.reserve(N);

    wrapper::set_max_threads(m_method, m_num_threads);
    for (int i = 0; i < m_num_threads; i++) {
        threads.emplace_back(std::thread([this, &distances, chunk_size, & external_ids, N] (int thread_id) {
            wrapper::init_thread(m_method, thread_id);
            uint64_t start = thread_id * chunk_size;
            uint64_t size = N;
            uint64_t end = start + chunk_size;
            if (end > size) end = size;
            
            auto snapshot_local = wrapper::snapshot_clone(m_snapshot);

            for (int64_t u = start; u < end; u++) {
                external_ids[u] = std::make_pair(wrapper::snapshot_physical2logical(snapshot_local, u), distances[u].load());
                // std::cout << u << ' ' << distances[u].load() << std::endl;
            }

            wrapper::end_thread(m_method, thread_id);
        }, i));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    for (int64_t u = 0; u < distances.size(); u++) {
        external_ids[u] = std::make_pair(wrapper::snapshot_physical2logical(m_snapshot, u), distances[u].load());
        // std::cout << u << ' ' << distances[u].load() << std::endl;
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Gapbs BFS took " << duration.count() << " milliseconds" << std::endl;
    log_info("BFS: %ld milliseconds", duration.count());
    
}
#endif // BFS_HPP
