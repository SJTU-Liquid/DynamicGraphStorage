#pragma once

#include <vector>
#include <cstdint>
#include <algorithm>
#include <atomic>
#include <iostream>
#include "utils/config.hpp"
#include "utils/types.hpp"
#include "types/types.hpp"

using PUU = std::pair<uint64_t, uint64_t>;

namespace container {
    template<typename Container>
    struct ReadTransaction;

    template<typename Container>
    struct WriteTransaction;

    template<typename Container>
    struct TransactionManager {
        std::atomic<bool> is_writing {false};
    public:
        Container* container_impl;

        explicit TransactionManager(bool is_directed, bool is_weighted){
            container_impl = new Container(is_directed, is_weighted);
        }

        ~TransactionManager() {
            delete container_impl;
        }

        auto get_write_transaction() {
            while (is_writing.exchange(true, std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            
            return WriteTransaction(this->container_impl, this);
        }

        auto get_read_transaction() const {
            return ReadTransaction<Container>(this->container_impl, this);
        }
    };

    template<typename Container>
    struct ReadTransaction {
        Container* container_impl;
        const TransactionManager<Container>* tm;

        // Functions
        ReadTransaction(Container* container_impl, const TransactionManager<Container>* tm): container_impl(container_impl), tm(tm) {}

        bool has_vertex(uint64_t vertex) const {
            auto res = container_impl->has_vertex(vertex);
            return res;
        }

        bool has_edge(uint64_t source, uint64_t destination) const {
            auto res = container_impl->has_edge(source, destination);
            return res;
        }



        uint64_t intersect(uint64_t vtx_a, uint64_t vtx_b) const {
            auto res = container_impl->intersect(vtx_a, vtx_b);
            return res;
        }

        uint64_t get_degree(uint64_t source) const {
            auto res = container_impl->get_degree(source);
            return res;
        }

        void get_neighbor(uint64_t src, std::vector<uint64_t> &neighbor) const {
            container_impl->get_neighbor(src, neighbor);
        }

        void get_neighbor_ptr(uint64_t vertex) const {
            volatile void* addr = container_impl->get_neighbor_ptr(vertex);
        }

        template<typename F>
        void edges(uint64_t src, F&& callback) const {
            container_impl->edges(src, callback);
        }

        void get_vertices(std::vector<uint64_t> &vertices) {
            container_impl->get_vertices(vertices);
        }


        auto begin(uint64_t src) const {
            auto it = container_impl->begin(src);
            return it;
        }

        bool commit() {
            return true;
        }
    };

    /// Write transaction, Single writer
    template<typename Container>
    struct WriteTransaction {
        Container* container_impl;
        TransactionManager<Container>* tm;

        std::vector<uint64_t> vertex_insert_vec{};
        std::vector<PUU> edge_insert_vec{};


        // Functions
        WriteTransaction(Container* container_impl, TransactionManager<Container>* tm): container_impl(container_impl), tm(tm) {}

        void insert_vertex(uint64_t vertex) {
            vertex_insert_vec.push_back(vertex);
        }

        /// Note: the src. and dest. must been inserted transaction where the edge is inserted
        void insert_edge(uint64_t source, uint64_t destination) {
            edge_insert_vec.emplace_back(source, destination);
        }

        void insert_edge_batch(std::vector<operation> op, uint64_t start, uint64_t end) {
            container_impl->insert_edge_batch(op, start, end);
        }

        /// Hazard! This function is not thread-safe
        void clear() {
            container_impl->clear();
        }

        bool commit() {
            // insert vertex
            for (auto& vertex: vertex_insert_vec) {
                if (!container_impl->insert_vertex(vertex)) {
                    throw std::runtime_error("insert vertex failed");
                }
            }

            // insert edge
            for (auto& edge: edge_insert_vec) {
                if (!container_impl->insert_edge(edge.first, edge.second)) {
                    throw std::runtime_error("insert edge failed");
                }
            }

            tm->is_writing.store(false);
            return true;
        }

        void abort() {
        }
    };
}
