#pragma once

#include <vector>
#include <limits>
#include <algorithm>
#include <memory>

#include "utils/types.hpp"
#include "utils/config.hpp"
#include "utils/PAM/include/pam/pam.h"
#include "utils/unordered_dense/include/ankerl/unordered_dense.h"


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
            auto get_neighbor_ptr = [&] (typename pam_map<VertexEntry<EdgeIndex, EdgeEntry>>::E& l) -> void {
                callback(l.first);
            };
            pam_map<VertexEntry<EdgeIndex, EdgeEntry>, avl_tree>::foreach_seq(m_vertex_index, get_neighbor_ptr);
        }

        void get_neighbor_ptrs(std::vector<EdgeIndex<EdgeEntry> *> &neighbor_ptrs) const {
            if (neighbor_ptrs.size() < m_vertex_index.size()) {
                neighbor_ptrs.resize(m_vertex_index.size());
            }
            auto get_neighbor_ptr = [&] (typename pam_map<PAMVertexEntry<EdgeIndex, EdgeEntry>>::E& l) -> void {
                neighbor_ptrs[l.first] = l.second;
            };
            pam_map<PAMVertexEntry<EdgeIndex, EdgeEntry>, avl_tree>::foreach_seq(m_vertex_index, get_neighbor_ptr);
        }

        void get_neighbor_ptrs_map(std::shared_ptr<ankerl::unordered_dense::map<uint64_t, void*>> neighbor_ptrs) const {
            if (neighbor_ptrs->size() < m_vertex_index.size()) {
                neighbor_ptrs->reserve(m_vertex_index.size());
            }
            auto get_neighbor_ptr = [&] (typename pam_map<PAMVertexEntry<EdgeIndex, EdgeEntry>>::E& l) -> void {
                neighbor_ptrs->insert(std::make_pair(l.first, l.second));
            };
            pam_map<PAMVertexEntry<EdgeIndex, EdgeEntry>, avl_tree>::foreach_seq(m_vertex_index, get_neighbor_ptr);
        }

        bool insert_vertex(uint64_t vertex, EdgeIndex<EdgeEntry>* neighbor_ptr) {
            m_vertex_index.insert({vertex, neighbor_ptr});
            return true;
        }

        EdgeIndex<EdgeEntry>* get_neighbor_ptr(uint64_t vertex) const {
            auto res = m_vertex_index.find(vertex);
            if (!res.has_value()) {
                return nullptr;
            }
            return res.value();
        }

        void* get_entry(uint64_t vertex) const {
            auto res = m_vertex_index.find(vertex);
            if (!res.has_value()) {
                return nullptr;
            }
            return res.value();
        }

        void clear() {
            m_vertex_index.clear();
        }

        pam_map<PAMVertexEntry<EdgeIndex, EdgeEntry>, avl_tree> m_vertex_index;
    };
}