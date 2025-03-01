#pragma once

#include <vector>
#include <limits>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include "utils/unordered_dense/include/ankerl/unordered_dense.h"

#include "utils/types.hpp"
#include "utils/config.hpp"

namespace container {
    template<
        template<template<typename> class, typename> class VertexEntry,
        template<typename> class EdgeIndex,
        typename EdgeEntry
    >
    struct HashmapVertexIndex {
        HashmapVertexIndex() = default;

        bool lock(container::RequiredLock& lock) {
            if (lock.idx == container::config::VERTEX_INDEX_LOCK_IDX) {
                return true;
            }
            auto iter = m_vertex_index.find(lock.idx);
            if (iter == m_vertex_index.end()) {
                throw std::runtime_error("Lock not found");
            }
            if (lock.is_exclusive) {
                iter->second.lock();
            } else {
                iter->second.lock_shared();
            }
            return true;
        }

        bool unlock(container::RequiredLock& lock) {
            if (lock.idx == container::config::VERTEX_INDEX_LOCK_IDX) {
                return true;
            }
            // find idx
            auto iter = m_vertex_index.find(lock.idx);
            if (lock.is_exclusive) {
                iter->second.unlock();
            } else {
                iter->second.unlock_shared();
            }
            return true;
        }

        bool has_vertex(uint64_t vertex) {
            return m_vertex_index.find(vertex) != m_vertex_index.end();
        }

        bool insert_vertex(uint64_t vertex, NeighborEntry<EdgeIndex, EdgeEntry>* neighbor_ptr, uint64_t timestamp = 0) {
            VertexEntry<EdgeIndex, EdgeEntry> vertex_entry;
            vertex_entry.vertex = vertex;
            vertex_entry.neighbor = neighbor_ptr;
            vertex_entry.update_degree(0, timestamp);
            m_vertex_index[vertex] = VertexEntry<EdgeIndex, EdgeEntry>{vertex, timestamp, neighbor_ptr};
            return true;
        }

        NeighborEntry<EdgeIndex, EdgeEntry>* get_neighbor_ptr(uint64_t vertex) const {
            auto iter = m_vertex_index.find(vertex);
            if (iter == m_vertex_index.end()) {
                throw std::runtime_error("Vertex not found");
            }
            return iter->second.neighbor;
        }

        template<typename F>
        void scan(F &&callback) const {
            uint64_t sz = m_vertex_index.size();
            for (uint64_t i = 0; i < sz; i++) {
                auto iter = m_vertex_index.find(i);
                callback(iter->second.vertex);
            }
        }

        void get_neighbor_ptrs(std::vector<EdgeIndex<EdgeEntry> *> &neighbor_ptrs) const {
            for (auto & iter : m_vertex_index) {
                neighbor_ptrs.push_back(iter.second.neighbor);
            }
        }

        VertexEntry<EdgeIndex, EdgeEntry>* get_entry(uint64_t vertex) {
            auto iter = m_vertex_index.find(vertex);
            if (iter == m_vertex_index.end()) {
                throw std::runtime_error("Vertex not found");
            }
            return &iter->second;
        }

        void get_vertices(std::vector<uint64_t>& vertices) {
            for (auto & iter : m_vertex_index) {
                vertices.push_back(iter.second.vertex);
            }
        }

        void clear() {
            for (auto & iter : m_vertex_index) {
                iter.second.clear();
            }
            m_vertex_index.clear();
        }

        ankerl::unordered_dense::map<uint64_t, VertexEntry<EdgeIndex, EdgeEntry>> m_vertex_index;
    };
}