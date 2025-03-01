#pragma once

#include <cstdint>
#include <string>
#include <iostream>
#include <vector>
#include <functional>
#include "utils/types.hpp"
#include "types/types.hpp"

namespace container {
    template<template<typename> class EdgeIndexTemplate, typename EdgeEntry>
    concept EdgeIndexConcept = requires {
        typename EdgeIndexTemplate<EdgeEntry>;
    };

    template<template<typename> class EdgeIndexTemplate, typename EdgeEntry,
            template<template<typename> class, typename> class VertexEntryTemplate>
    concept VertexEntryConcept = requires {
        typename VertexEntryTemplate<EdgeIndexTemplate, EdgeEntry>;
        requires EdgeIndexConcept<EdgeIndexTemplate, EdgeEntry>;
    };

    template<template<typename, template<typename> class, typename> class VertexIndexTemplate,
            template<template<typename> class, typename> class VertexEntryTemplate,
            template<typename> class EdgeIndexTemplate, typename EdgeEntry>
    concept VertexIndexConcept = requires {
        typename VertexIndexTemplate<VertexEntryTemplate<EdgeIndexTemplate, EdgeEntry>, EdgeIndexTemplate, EdgeEntry>;
        requires VertexEntryConcept<EdgeIndexTemplate, EdgeEntry, VertexEntryTemplate>;
    };
    template<
        template<typename, template<typename> class, typename> class VertexIndexTemplate,
        template<template<typename> class, typename> class VertexEntryTemplate,
        template<typename> class EdgeIndexTemplate,
        typename EdgeEntry>
    requires VertexIndexConcept<VertexIndexTemplate, VertexEntryTemplate, EdgeIndexTemplate, EdgeEntry>
        && VertexEntryConcept<EdgeIndexTemplate, EdgeEntry, VertexEntryTemplate>
        && EdgeIndexConcept<EdgeIndexTemplate, EdgeEntry>

    class Container {
        using VertexEntry = VertexEntryTemplate<EdgeIndexTemplate, EdgeEntry>;
        using VertexIndex = VertexIndexTemplate<VertexEntry, EdgeIndexTemplate, EdgeEntry>;
        using EdgeIndex = EdgeIndexTemplate<EdgeEntry>;
    
    public:
        // Ctor & Dtor
        Container(bool is_directed, bool is_weighted): m_is_directed(is_directed), m_is_weighted(is_weighted), m_vertex_count(0), m_edge_count(0) {
            this->vertex_index = new VertexIndex();
        }

        /// We use clear() to avoid memory leak
        ~Container() {
            clear();
            delete vertex_index;
        }

        void acquire_locks(std::vector<container::RequiredLock>& locks_required) {
            for(auto& lock: locks_required) {
                vertex_index->lock(lock);
            }
        }

        void acquire_lock_shared(container::RequiredLock lock) {
            vertex_index->lock(lock);
        }

        void release_locks(std::vector<container::RequiredLock>& locks_required) {
            for(auto& lock: locks_required) {
                vertex_index->unlock(lock);
            }
        }

        void release_lock_shared(container::RequiredLock lock) {
            vertex_index->unlock(lock);
        }

        // Graph Operations, locks has been acquired
        bool is_directed() const {
            return m_is_directed;
        }

        bool is_weighted() const {
            return m_is_weighted;
        }

        bool is_empty() const {
            return m_vertex_count == 0;
        }

        uint64_t vertex_count() const {
            return m_vertex_count;
        }

        uint64_t edge_count() const {
            return m_edge_count;
        }

        bool has_vertex(uint64_t vertex) const {
            return vertex_index->has_vertex(vertex);
        }

        bool has_edge(uint64_t src, uint64_t dest, uint64_t timestamp) const {
            auto ptr = vertex_index->get_neighbor_ptr(src);
            if (!has_vertex(dest) || ptr == nullptr) {
                return false;
            }
            return ptr->has_edge(src, dest, timestamp);
        }

        uint64_t intersect(uint64_t vtx_a, uint64_t vtx_b, uint64_t timestamp) const {
            auto ptr_a = vertex_index->get_neighbor_ptr(vtx_a);
            auto ptr_b = vertex_index->get_neighbor_ptr(vtx_b);
            return ptr_a->intersect(*ptr_b, timestamp);
        }

        auto begin(uint64_t src, uint64_t timestamp) const {
            auto ptr = vertex_index->get_neighbor_ptr(src);
            return ptr->get_begin(timestamp);
        }

        uint64_t get_degree(uint64_t src, uint64_t timestamp) const {
            auto ptr = vertex_index->get_entry(src);
            if (ptr == nullptr) {
                throw std::runtime_error("Vertex does not exist");
            }
            return ptr->get_degree(timestamp);
        }

        void get_neighbor(uint64_t src, std::vector<uint64_t>& neighbor, uint64_t timestamp) const {
            auto ptr = vertex_index->get_neighbor_ptr(src);
            if (ptr == nullptr) {
                throw std::runtime_error("Vertex does not exist");
            }
            ptr->get_neighbor(neighbor, timestamp);
        }

        bool insert_vertex(uint64_t vertex, uint64_t timestamp) {
            auto ptr = new NeighborEntry<EdgeIndexTemplate, EdgeEntry>();
            vertex_index->insert_vertex(vertex, ptr, timestamp);
            m_vertex_count++;
            return true;
        }

        bool insert_edge(uint64_t src, uint64_t dest, uint64_t timestamp) {
            auto ptr = vertex_index->get_neighbor_ptr(src);
            if(ptr == nullptr) {
                throw std::runtime_error("Vertex does not exist");
            }
            bool flag = ptr->insert_edge(dest, timestamp);
            if (flag) {
                m_edge_count++;
                
                // update degree
                auto vertex_ptr = vertex_index->get_entry(src);
                auto cur_degree = vertex_ptr->get_degree(timestamp);
                vertex_ptr->update_degree(cur_degree + 1, timestamp);
            }
            return flag;
        }


        bool insert_edge_batch(std::vector<operation> & stream, uint64_t start, uint64_t end) {
            std::vector<std::vector<uint64_t>> graph(m_vertex_count);
            for (uint64_t i = start; i < end; i++) {
                auto op = stream[i];
                auto edge = op.e;
                graph[edge.source].push_back(edge.destination);
                if(!m_is_directed) graph[edge.destination].push_back(edge.source);
            }
            for (uint64_t i = 0; i < m_vertex_count; i++) {
                auto ptr = vertex_index->get_neighbor_ptr(i);
                if(ptr == nullptr) {
                    throw std::runtime_error("Vertex does not exist");
                }
                ptr->init_graph(graph[i], 0, graph[i].size());
            }
            if (m_is_directed) m_edge_count += end - start;
            else m_edge_count += 2 * (end-start);
            return true;
        }

        void clear() {
            std::vector<EdgeIndex*> neighbor_ptrs;
            vertex_index->get_neighbor_ptrs(neighbor_ptrs);
            for(auto &neighbor_ptr: neighbor_ptrs) {
                neighbor_ptr->clear_neighbor();
            }

            vertex_index->clear();
            m_vertex_count = 0;
            m_edge_count = 0;
        }

        void get_vertices(std::vector<uint64_t> &vertices, uint64_t timestamp) const {
            vertex_index->get_vertices(vertices);
        }

        void* get_neighbor_ptr(uint64_t vertex) const {
            return vertex_index->get_neighbor_ptr(vertex);
        }

        template<typename F>
        void edges(uint64_t src, F&& callback, uint64_t timestamp) const {
            auto ptr = vertex_index->get_neighbor_ptr(src);
            if (ptr == nullptr) {
                throw std::runtime_error("Vertex does not exist");
            }
            ptr->edges(callback, timestamp);
        }

    private:
        const bool m_is_directed;
        const bool m_is_weighted;
        std::atomic<uint64_t> m_vertex_count {0};
        std::atomic<uint64_t> m_edge_count {0};

        VertexIndex* vertex_index;
        // EdgeIndex* edge_index;
    };
}