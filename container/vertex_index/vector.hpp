#pragma once

#include <vector>
#include <limits>
#include <algorithm>
#include <memory>
#include <stdexcept>

#include "vertex_index.cpp"
#include "utils/types.hpp"
#include "utils/config.hpp"
namespace container {
    /// Vector-based vertex indexor
    template<
        template<template<typename> class, typename> class VertexEntry,
        template<typename> class EdgeIndex,
        typename EdgeEntry
    >
    struct VectorVertexIndex {
        VectorVertexIndex() {
            m_vertex_index = new std::vector<VertexEntry<EdgeIndex, EdgeEntry>>();
        }

        ~VectorVertexIndex() {
            if (m_vertex_index) {
                for (auto &vertex : *m_vertex_index) {
                    vertex.clear();
                }
                delete m_vertex_index;
            }
        }

        bool lock(container::RequiredLock& lock) {
            if (lock.idx == container::config::VERTEX_INDEX_LOCK_IDX) {
                if (lock.is_exclusive) {
                    m_lock.lock();
                } else {
                    m_lock.lock_shared();
                }
            }
            else {
                if (lock.idx >= m_vertex_index->size()) {
                    throw std::runtime_error("Lock not found");
                }
                if (lock.is_exclusive) {
                    (*m_vertex_index)[lock.idx].lock();
                } else {
                    (*m_vertex_index)[lock.idx].lock_shared();
                }
            }
            return true;
        }

        bool unlock(container::RequiredLock& lock) {
            if (lock.idx == container::config::VERTEX_INDEX_LOCK_IDX) {
                if (lock.is_exclusive) {
                    m_lock.unlock();
                } else {
                    m_lock.unlock_shared();
                }
            }
            else {
                if (lock.is_exclusive) {
                    (*m_vertex_index)[lock.idx].unlock();
                } else {
                    (*m_vertex_index)[lock.idx].unlock_shared();
                }
            }
            return true;
        }

        bool has_vertex(uint64_t vertex) const {
            return vertex < size;
        }

        void get_vertices(std::vector<uint64_t>& vertices) const {
            for(auto& vertex: *m_vertex_index) {
                if (vertex.vertex != std::numeric_limits<uint64_t>::max()) {
                    vertices.push_back(vertex.vertex);
                }
            }
        }

        template<typename F>
        void scan(F &&callback) const { // TODO: change this to foreach_xx
            for(auto& vertex: *m_vertex_index) {
                callback(vertex.vertex);
            }
        }

        template<typename F>
        void foreach_entry(F &&callback) const {
            for(auto& vertex: *m_vertex_index) {
                callback(vertex);
            }
        }

        bool insert_vertex(uint64_t vertex, NeighborEntry<EdgeIndex, EdgeEntry>* neighbor_ptr, uint64_t timestamp = 0) {
            VertexEntry<EdgeIndex, EdgeEntry> vertex_entry;
            vertex_entry.vertex = size++;
            vertex_entry.neighbor = neighbor_ptr;
            vertex_entry.update_degree(0, timestamp);
            m_vertex_index->push_back(std::move(vertex_entry));
            return true;
        }

        NeighborEntry<EdgeIndex, EdgeEntry>* get_neighbor_ptr(uint64_t vertex) const {
            return (*m_vertex_index)[vertex].neighbor;
        }

        VertexEntry<EdgeIndex, EdgeEntry>* get_entry(uint64_t vertex) const {
            return &(*m_vertex_index)[vertex];
        }

        void clear() {
            if (m_vertex_index) {
                for(auto& vertex: *m_vertex_index) {
                    vertex.clear();
                }

                m_vertex_index->clear();
                delete m_vertex_index;
            }
        }

        std::vector<VertexEntry<EdgeIndex, EdgeEntry>> *m_vertex_index;
        RWSpinLock m_lock;
        uint64_t size = 0;
    };
}