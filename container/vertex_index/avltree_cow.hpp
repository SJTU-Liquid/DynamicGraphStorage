#pragma once

#include <vector>
#include <limits>
#include <algorithm>
#include <memory>
#include <tbb/parallel_for.h>
#include <unordered_map>

#include "utils/types.hpp"
#include "utils/config.hpp"
#include "utils/PAM/include/pam/pam.h"
#include "utils/unordered_dense/include/ankerl/unordered_dense.h"

using PUU = std::pair<uint64_t, uint64_t>;

namespace container {
    template<
        template<template<typename> class, typename> class VertexEntry,
        template<typename> class EdgeIndex,
        typename EdgeEntry
    >
    struct AVLTreeVertexIndex {
        AVLTreeVertexIndex() = default;

        bool has_vertex(uint64_t vertex) const {
            return m_vertex_index.find(vertex).has_value();
        }

        void get_vertices(std::vector<uint64_t>& vertices) const {
            if (vertices.size() < m_vertex_index.size()) {
                vertices.resize(m_vertex_index.size());
            }
            pam_map<VertexEntry<EdgeIndex, EdgeEntry>, avl_tree>::keys(m_vertex_index, vertices.begin());
        }

        template<typename F>
        void scan(F &&callback) const {
            auto get_neighbor = [&] (typename pam_map<VertexEntry<EdgeIndex, EdgeEntry>>::E& l) -> void {
                callback(l.first);
            };
            pam_map<VertexEntry<EdgeIndex, EdgeEntry>, avl_tree>::foreach_seq(m_vertex_index, get_neighbor);
        }

        void get_neighbor_ptrs(std::vector<EdgeIndex<EdgeEntry>> &neighbor_ptrs) const {
            if (neighbor_ptrs.size() < m_vertex_index.size()) {
                neighbor_ptrs.resize(m_vertex_index.size());
            }
            auto get_neighbor = [&] (typename pam_map<VertexEntry<EdgeIndex, EdgeEntry>>::E& l) -> void {
                neighbor_ptrs[l.first] = l.second;
            };
            pam_map<VertexEntry<EdgeIndex, EdgeEntry>, avl_tree>::foreach_seq(m_vertex_index, get_neighbor);
        }

        void get_neighbor_ptrs_map(std::shared_ptr<ankerl::unordered_dense::map<uint64_t, void*>> neighbor_ptrs) const {
            if (neighbor_ptrs->size() < m_vertex_index.size()) {
                neighbor_ptrs->reserve(m_vertex_index.size());
            }
            auto get_neighbor = [&] (typename pam_map<VertexEntry<EdgeIndex, EdgeEntry>>::E& l) -> void {
                neighbor_ptrs->insert(std::make_pair(l.first, l.second));
            };
            pam_map<VertexEntry<EdgeIndex, EdgeEntry>, avl_tree>::foreach_seq(m_vertex_index, get_neighbor);
        }

        bool insert_vertex(uint64_t vertex) {
            m_vertex_index.insert({vertex, std::move(EdgeIndex<EdgeEntry>(vertex))});
            return true;
        }

        void update_vertex(uint64_t vertex, EdgeIndex<EdgeEntry> neighbor) {
            auto update_func = [&] (std::pair<uint64_t, container::PAMStruct> entry) {
                return neighbor;
            };
            m_vertex_index = m_vertex_index.update(m_vertex_index, vertex, update_func);
        }

        bool insert_edge(uint64_t src, uint64_t dest) {
            bool flag = false;
            auto update_func = [&] (std::pair<uint64_t, container::PAMStruct>& entry) { // TODO: use template
                return std::move(entry.second.insert_edge(dest, flag));
            };
            m_vertex_index.update(src, update_func);
            
            return flag;
        }

        // need to make sure edge list is sorted!!!
        uint64_t insert_edge_batch(const std::vector<PUU>& edge_list) {
            std::atomic<uint64_t> edges_added = 0;
            if (edge_list.empty()) return 0;
            
            ankerl::unordered_dense::map<uint64_t, PUU> csr_row_table;
            std::vector<uint64_t> kv;

            uint64_t estimate_num_vertices = edge_list.size() >> 1;
            csr_row_table.reserve(estimate_num_vertices);
            kv.reserve(estimate_num_vertices);

            uint64_t pre_src = edge_list[0].first;
            csr_row_table.emplace(pre_src, PUU{0, 0});
            kv.emplace_back(pre_src);

            for (size_t i = 0; i < edge_list.size(); ++i) {
                uint64_t src = edge_list[i].first;
                if (src != pre_src) {
                    // Update end index for previous source vertex
                    csr_row_table[pre_src].second = i;
                    // Insert new source vertex with start index
                    csr_row_table.emplace(src, PUU{i, 0});
                    kv.emplace_back(src);
                    pre_src = src;
                }
            }

            csr_row_table[pre_src].second = edge_list.size();

            auto update_func = [&] (container::PAMStruct &&a) { 
                auto cur_source = a.vertex_id;
                auto &pair = csr_row_table[cur_source];
                auto start = pair.first, end = pair.second;
                uint64_t degree_added = 0;
                auto new_edge_index = a.insert_edge_batch(edge_list, start, end, degree_added);
                edges_added.fetch_add(degree_added);
                return new_edge_index;
            };

#ifdef ENABLE_PARLAY
            auto new_index = m_vertex_index.multi_update_sorted_neo(std::move(m_vertex_index), kv, std::move(update_func));
            m_vertex_index = new_index;
#else
            for (auto iter = csr_row_table.begin(); iter != csr_row_table.end(); iter++) {
                auto src = iter->first;
                auto update_func = [&] (std::pair<uint64_t, container::PAMStruct>& entry) { // TODO: use template
                    // auto & adj_list = graph[csr_row_table[src]];
                    // bool flag = false;
                    auto &pair = csr_row_table[src];
                    auto start = pair.first, end = pair.second;
                    uint64_t degree_added = 0;
                    auto new_edge_index = std::move(entry.second.insert_edge_batch(edge_list, start, end, degree_added));
                    edges_added.fetch_add(degree_added);
                    return std::move(new_edge_index);
                };
                m_vertex_index.update(src, update_func);
            }
#endif
            return edges_added;
        }

        EdgeIndex<EdgeEntry> get_neighbor(uint64_t vertex) const {
            auto res = m_vertex_index.find(vertex);
            
            return res.value();
        }

        void get_entry(uint64_t vertex) const {
            auto res = m_vertex_index.find(vertex);
            
            return res.value();
        }

        void clear() {
            m_vertex_index.clear();
        }

        pam_map<VertexEntry<EdgeIndex, EdgeEntry>, avl_tree> m_vertex_index;
    };
}