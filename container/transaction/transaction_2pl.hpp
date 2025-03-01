#pragma once

#include <thread>
#include <chrono>
#include <pthread.h>
#include <vector>
#include <unordered_set>
#include <cstdint>
#include <algorithm>
#include <atomic>
#include <iostream>
#include <mutex>
#include <tbb/parallel_sort.h>
#include "utils/config.hpp"
#include "utils/types.hpp"
#include "types/types.hpp"

using PUU = std::pair<uint64_t, uint64_t>;

namespace container {
    template<typename Container>
    struct ReadTransaction;

    template<typename Container>
    struct WriteTransaction;

    struct ActiveVersionManager {
    private:
        std::atomic<uint64_t> min_timestamp {0};
        std::unordered_multiset<uint64_t> active_read_transactions;
        std::mutex mtx;
    public:
        void begin_read_transaction(uint64_t timestamp) {
            std::lock_guard<std::mutex> lock(mtx);
            active_read_transactions.insert(timestamp);
        } 

        void end_read_transaction(uint64_t timestamp) {
            std::lock_guard<std::mutex> lock(mtx);
            auto it = active_read_transactions.find(timestamp);
            if (it != active_read_transactions.end()) {
                active_read_transactions.erase(it);
            }
        }

        uint64_t get_min_timestamp() const {
            return min_timestamp.load(std::memory_order_acquire);
        }

        void update_min_version(uint64_t global_timestamp) {
            std::lock_guard<std::mutex> lock(mtx);
            if (active_read_transactions.empty()) {
                min_timestamp.store(global_timestamp, std::memory_order_release); 
            } else {
                auto min_ts = *std::min_element(active_read_transactions.begin(), active_read_transactions.end());
                min_timestamp.store(min_ts, std::memory_order_release);
            }
            // std::cout << min_timestamp << ' ' << global_timestamp << std::endl;
        }
    };

    template<typename Container>
    struct TransactionManager {
        std::atomic<uint64_t> global_timestamp {0};
        mutable ActiveVersionManager version_manager;

        std::atomic<bool> stopped {true};
        std::thread gc_all;
        Container* container_impl;

        explicit TransactionManager(bool is_directed, bool is_weighted)
            : gc_all(&TransactionManager::garbage_collector, this, 5000)
        {
            container_impl = new Container(is_directed, is_weighted);
            stopped.store(false);
        }

        ~TransactionManager() {
            stopped.store(true);
            if(gc_all.joinable()) gc_all.join();
            delete container_impl;
        }

        auto get_timestamp() {
            return global_timestamp;
        }

        auto get_write_transaction() {
            return WriteTransaction(this->container_impl, this);
        }

        auto get_read_transaction() const {
            uint64_t timestamp = global_timestamp.load();
            version_manager.begin_read_transaction(timestamp);
            return ReadTransaction<Container>(this->container_impl, this, timestamp);
        }

        void garbage_collector(uint interval) {
#ifdef ENABLE_GC
            while (!stopped.load()) {
                this->version_manager.update_min_version(global_timestamp.load());
                this->container_impl->gc_all(version_manager.get_min_timestamp());
                std::this_thread::sleep_for(std::chrono::milliseconds(interval));
            }
#endif
        }
    };

    template<typename Container>
    struct ReadTransaction {
        Container* container_impl;
        const TransactionManager<Container>* tm;

        std::vector<container::RequiredLock> locks_required{};    // exclusive locks

        const uint64_t timestamp;

        // Functions
        ReadTransaction(Container* container_impl, const TransactionManager<Container>* tm, uint64_t timestamp)
            : container_impl(container_impl), tm(tm), timestamp(timestamp) {}

        bool has_vertex(uint64_t vertex) const {
#ifdef ENABLE_LOCK
            container_impl->acquire_lock_shared({vertex, false});
#endif
            auto res = container_impl->has_vertex(vertex);
#ifdef ENABLE_LOCK
            container_impl->release_lock_shared({vertex, false});
#endif
            return res;
        }

        bool has_edge(uint64_t source, uint64_t destination) const {
#ifdef ENABLE_LOCK
            container_impl->acquire_lock_shared({source, false});
#endif
            auto res = container_impl->has_edge(source, destination, timestamp);
#ifdef ENABLE_LOCK
            container_impl->release_lock_shared({source, false});
#endif
            return res;
        }



        uint64_t intersect(uint64_t vtx_a, uint64_t vtx_b) const {
#ifdef ENABLE_LOCK
            container_impl->acquire_lock_shared({vtx_a, false});
            container_impl->acquire_lock_shared({vtx_b, false});
#endif
            auto res = container_impl->intersect(vtx_a, vtx_b, timestamp);
#ifdef ENABLE_LOCK
            container_impl->acquire_lock_shared({vtx_a, false});
            container_impl->acquire_lock_shared({vtx_b, false});
#endif
            return res;
        }

        auto begin(uint64_t src) const {
#ifdef ENABLE_LOCK
            container_impl->acquire_lock_shared({src, false});
#endif
            auto it = container_impl->begin(src, timestamp);
            return it;
        }

        uint64_t get_degree(uint64_t source) const {
#ifdef ENABLE_LOCK
            container_impl->acquire_lock_shared({source, false});
#endif
            auto res = container_impl->get_degree(source, timestamp);
#ifdef ENABLE_LOCK
            container_impl->release_lock_shared({source, false});
#endif
            return res;
        }

        void get_neighbor(uint64_t src, std::vector<uint64_t> &neighbor) const {
#ifdef ENABLE_LOCK
            container_impl->acquire_lock_shared({container::config::VERTEX_INDEX_LOCK_IDX, false});
            container_impl->acquire_lock_shared({src, false});
#endif
            container_impl->get_neighbor(src, neighbor, timestamp);
#ifdef ENABLE_LOCK
            container_impl->release_lock_shared({container::config::VERTEX_INDEX_LOCK_IDX, false});
            container_impl->release_lock_shared({src, false});
#endif
        }

        void get_neighbor_ptr(uint64_t vertex) const {
#ifdef ENABLE_LOCK
            container_impl->acquire_lock_shared({container::config::VERTEX_INDEX_LOCK_IDX, false});
            container_impl->acquire_lock_shared({vertex, false});
#endif
            volatile void* addr = container_impl->get_neighbor_ptr(vertex);
#ifdef ENABLE_LOCK
            container_impl->release_lock_shared({container::config::VERTEX_INDEX_LOCK_IDX, false});
            container_impl->release_lock_shared({vertex, false});
#endif
        }

        template<typename F>
        void edges(uint64_t src, F&& callback) const {
#ifdef ENABLE_LOCK
            container_impl->acquire_lock_shared({src, false});
#endif
            container_impl->edges(src, callback, timestamp);
#ifdef ENABLE_LOCK
            container_impl->release_lock_shared({src, false});
#endif
        }

        void get_vertices(std::vector<uint64_t> &vertices) {
#ifdef ENABLE_LOCK
            container_impl->acquire_lock_shared({container::config::VERTEX_INDEX_LOCK_IDX, false});
#endif
            container_impl->get_vertices(vertices, timestamp);
#ifdef ENABLE_LOCK
            container_impl->release_lock_shared({container::config::VERTEX_INDEX_LOCK_IDX, false});
#endif
        }


        uint64_t edge_count() const {
            return container_impl->edge_count(timestamp);
        }

        bool commit() {
            tm->version_manager.end_read_transaction(timestamp);
            return true;
        }
    };

    /// Write transaction, Single writer
    template<typename Container>
    struct WriteTransaction {
        Container* container_impl;
        TransactionManager<Container>* tm;

#ifdef ENABLE_LOCK
        std::vector<container::RequiredLock> locks_required{};    // exclusive locks
#endif
        std::vector<uint64_t> vertex_insert_vec{};
        std::vector<PUU> edge_insert_vec{};

        uint64_t timestamp;

        // Functions
        WriteTransaction(Container* container_impl, TransactionManager<Container>* tm): container_impl(container_impl), tm(tm), timestamp(0) {}

        void insert_vertex(uint64_t vertex) {
#ifdef ENABLE_LOCK
            locks_required.push_back({container::config::VERTEX_INDEX_LOCK_IDX, true});
#endif
            vertex_insert_vec.push_back(vertex);
        }

        /// Note: the src. and dest. must been inserted transaction where the edge is inserted
        void insert_edge(uint64_t source, uint64_t destination) {
#ifdef ENABLE_LOCK
            // locks_required.push_back({container::config::VERTEX_INDEX_LOCK_IDX, false});
            locks_required.push_back({source, true});
#endif
            edge_insert_vec.emplace_back(source, destination);
        }

        /// Hazard! This function is not thread-safe
        void clear() {
            container_impl->clear();
        }

#ifdef ENABLE_LOCK 
        void sort_and_unique() {
            std::sort(locks_required.begin(), locks_required.end(), [](const container::RequiredLock& a, const container::RequiredLock& b) {
                return a.idx < b.idx;
            });
            locks_required.erase(std::unique(locks_required.begin(), locks_required.end(), [](const container::RequiredLock& a, const container::RequiredLock& b) {
                return a.idx == b.idx;
            }), locks_required.end());
        }
#endif

        bool commit() {
#ifdef ENABLE_LOCK
            if (locks_required.size() == 2) {
                if (locks_required[0].idx == locks_required[1].idx) {
                    locks_required.pop_back();  // Remove the duplicate
                } else if (locks_required[0].idx > locks_required[1].idx) {
                    // If they are not equal, sort them in ascending order
                    std::swap(locks_required[0].idx, locks_required[1].idx);
                }
            }

            else if (locks_required.size() > 2) sort_and_unique();
            container_impl->acquire_locks(locks_required);
#endif

            timestamp = tm->global_timestamp.fetch_add(1) + 1;
            // insert vertex
            for (auto& vertex: vertex_insert_vec) {
                if (!container_impl->insert_vertex(vertex, timestamp)) {
                    std::cerr << "vertex exists, transaction_2pl::insert_vertex: " << vertex << std::endl;
                }
            }

            // insert single edge
            if (edge_insert_vec.empty()) return true;
            else if (edge_insert_vec.size() <= 2) { // TODO: change this ugly hardcode
                for (auto & edge : edge_insert_vec) {
                    if (!container_impl->insert_edge(edge.first, edge.second, timestamp)) {
                        std::cerr << "edge exists: transaction_2pl::insert_edge: " << edge.first << ' ' << edge.second << std::endl;
                    }
                }
            }
            // batch insert
            else {
                std::sort(edge_insert_vec.begin(), edge_insert_vec.end(), [] (const PUU& a, const PUU& b) {
                    if (a.first == b.first) return a.second < b.second;
                    else return a.first < b.first;
                });
                
                auto iter = edge_insert_vec.begin();
                while (iter != edge_insert_vec.end()) {
                    uint64_t cur_src = iter->first;
                    std::vector<uint64_t> dest_list;
                    while (iter != edge_insert_vec.end() && iter->first == cur_src) {
                        dest_list.push_back(iter->second);
                        iter++;
                    }
                    container_impl->insert_edge_batch(cur_src, dest_list, timestamp);
                }
            }

#ifdef ENABLE_LOCK
            container_impl->release_locks(locks_required);
#endif
            return true;
        }

        void abort() {
        }
    };
}
