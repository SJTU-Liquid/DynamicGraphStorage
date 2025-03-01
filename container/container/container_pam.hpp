#pragma once

#include <cstdint>
#include <string>
#include <iostream>
#include <vector>
#include <functional>
#include <omp.h>
#include <chrono>
#include "utils/types.hpp"
#include "utils/parallel_functions.hpp"
#include "types/types.hpp"
#include <tbb/parallel_for.h>
#include <tbb/parallel_sort.h>

using PUU = std::pair<uint64_t, uint64_t>;

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
        Container(bool is_directed, bool is_weighted): m_is_directed(is_directed), m_is_weighted(is_weighted), m_vertex_count(0), m_edge_count(0) {}

        /// We use clear() to avoid memory leak
        ~Container() {
            clear();
        }

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
            return vertex_index.has_vertex(vertex);
        }

        bool has_edge(uint64_t src, uint64_t dest) const {
            auto neighbor = vertex_index.get_neighbor(src);
            if (!has_vertex(dest)) {
                return false;
            }
            return neighbor.has_edge(src, dest);
        }

        uint64_t intersect(uint64_t vtx_a, uint64_t vtx_b) const {
            auto neighbor_a = vertex_index.get_neighbor(vtx_a);
            auto neighbor_b = vertex_index.get_neighbor(vtx_b);
            return neighbor_a.intersect(neighbor_b);
        }

        auto begin(uint64_t src) const {
            auto neighbor = vertex_index.get_neighbor(src);
            return neighbor.get_begin();
        }

        uint64_t get_degree(uint64_t src) const {
            auto neighbor = vertex_index.get_neighbor(src);
            
            return neighbor.get_degree();
            // TODO
        }

        void get_neighbor(uint64_t src, std::vector<uint64_t>& neighbor) const {
            auto neighbor_index = vertex_index.get_neighbor(src);
            neighbor_index.get_neighbor();
        }

        bool insert_vertex(uint64_t vertex) {
            bool flag = vertex_index.insert_vertex(vertex);
            if (flag) m_vertex_count++;
            return flag;
        }

        bool insert_edge(uint64_t src, uint64_t dest) {
            bool flag = vertex_index.insert_edge(src, dest);
            if (flag) m_edge_count++;
            return flag;
        }


        uint64_t insert_edge_batch(const std::vector<operation> & stream, uint64_t start, uint64_t end) {
            std::vector<PUU> edge_list;
            
            if (m_is_directed) {
                edge_list.resize((end - start));
#ifdef ENABLE_PARLAY
                tbb::parallel_for(start, end, [&](uint64_t i) {
                    edge_list[(i - start)] = {stream[i].e.source, stream[i].e.destination};
                });
#else
                for (uint64_t i = start; i < end; i++) {
                    edge_list[(i - start)] = {stream[i].e.source, stream[i].e.destination};
                }
#endif
            }
            else {
                edge_list.resize(2 * (end - start));
#ifdef ENABLE_PARLAY
                tbb::parallel_for(start, end, [&](uint64_t i) {
                    edge_list[(i - start) * 2] = {stream[i].e.source, stream[i].e.destination};
                    edge_list[(i - start) * 2 + 1] = {stream[i].e.destination, stream[i].e.source};
                });
#else
                for (uint64_t i = start; i < end; i++) {
                    edge_list[(i - start) * 2] = {stream[i].e.source, stream[i].e.destination};
                    edge_list[(i - start) * 2 + 1] = {stream[i].e.destination, stream[i].e.source};
                }
#endif
            }
#ifdef ENABLE_PARLAY
            // tbb::parallel_sort(edge_list.begin(), edge_list.end());
            sort_updates(edge_list);
#else
            std::sort(edge_list.begin(), edge_list.end());
#endif
            auto edges_added = vertex_index.insert_edge_batch(edge_list);
            m_edge_count += edges_added;

            return edges_added; // TODO: change this for duplicates
        }

        void clear() {
            vertex_index.clear();
            m_vertex_count = 0;
            m_edge_count = 0;
        }

        void get_vertices(std::vector<uint64_t> &vertices) const {
            vertex_index.get_vertices(vertices);
        }

        EdgeIndex get_neighbor(uint64_t vertex) const {
            return vertex_index.get_neighbor(vertex);
        }

        template<typename F>
        void edges(uint64_t src, F&& callback) const {
            auto neighbor = vertex_index.get_neighbor(src);
            
            neighbor.edges(neighbor, callback);
        }

        const bool m_is_directed;
        const bool m_is_weighted;
        std::atomic<uint64_t> m_vertex_count {0};
        std::atomic<uint64_t> m_edge_count {0};

        VertexIndex vertex_index;
    };
}