#pragma once

#include "../types/types.hpp"

#include "edge_index/pam_tree_cow.hpp"
#include "vertex_index/avltree_cow.hpp"


#include "transaction/transaction_pam.hpp"
#include "utils/types.hpp"
#include "utils/config.hpp"
#include "container/container_pam.hpp"

#include "wrapper.h"
#include "driver.h"

using EdgeEntry = uint64_t;

template<typename T>
using EdgeIndexTemplate = container::PAMStruct;

using EdgeIndex = EdgeIndexTemplate<EdgeEntry>;

template<template<typename> class EdgeIndexTemplate, typename EdgeEntry>
using VertexEntryTemplate = container::COWVertexEntry<EdgeIndexTemplate, EdgeEntry>;

template<typename VertexEntry, template<typename> class EdgeIndexTemplate, typename EdgeEntry>
using VertexIndexTemplate = container::AVLTreeVertexIndex<VertexEntryTemplate, EdgeIndexTemplate, EdgeEntry>;

using avl_tree2pam_container = container::Container<
    VertexIndexTemplate,
    VertexEntryTemplate,
    EdgeIndexTemplate,
    EdgeEntry
>;


class AVLTree_PAMTree_Wrapper {
private:
    container::TransactionManager<avl_tree2pam_container> tm;
    bool m_is_directed;
    bool m_is_weighted;
public:
    // Constructor
    explicit AVLTree_PAMTree_Wrapper(bool is_directed = false, bool is_weighted = true, size_t property_size = 8,
                                   int block_size = 1024)
            :tm(is_directed, is_weighted), m_is_weighted(is_weighted), m_is_directed(is_directed) {}

    AVLTree_PAMTree_Wrapper(const AVLTree_PAMTree_Wrapper &) = delete;

    AVLTree_PAMTree_Wrapper &operator=(const AVLTree_PAMTree_Wrapper &) = delete;

    ~AVLTree_PAMTree_Wrapper() = default;

    // Init

    std::shared_ptr<AVLTree_PAMTree_Wrapper> create_update_interface(const std::string &graph_type);

    // Multi-thread
    void set_max_threads(int max_threads) {};

    void init_thread(int thread_id) {};

    void end_thread(int thread_id) {};

    // Graph operations
    bool is_directed() const;

    bool is_weighted() const;

    bool is_empty() const;

    bool has_vertex(uint64_t vertex) const;

    bool has_edge(weightedEdge edge) const;

    bool has_edge(uint64_t source, uint64_t destination) const;

    bool has_edge(uint64_t source, uint64_t destination, double weight) const;

    uint64_t intersect(uint64_t vtx_a, uint64_t vtx_b) const;

    uint64_t degree(uint64_t vertex) const;

    double get_weight(uint64_t source, uint64_t destination) const;

    uint64_t logical2physical(uint64_t vertex) const;

    uint64_t physical2logical(uint64_t physical) const;

    uint64_t vertex_count() const;

    uint64_t edge_count() const;

    void get_neighbors(uint64_t vertex, std::vector<uint64_t> &) const;

    void get_neighbors(uint64_t vertex, std::vector<std::pair<uint64_t, double>> &) const;

    bool insert_vertex(uint64_t vertex);

    bool insert_edge(uint64_t source, uint64_t destination, double weight);

    bool insert_edge(uint64_t source, uint64_t destination);

    bool remove_vertex(uint64_t vertex);

    bool remove_edge(uint64_t source, uint64_t destination);

    bool run_batch_vertex_update(std::vector<uint64_t> &vertices, uint64_t start, uint64_t end);

    bool run_batch_edge_update(std::vector<operation> &edges, uint64_t start, uint64_t end, operationType type);

    void clear();

    // Snapshot Related
    class Snapshot  : public std::enable_shared_from_this<Snapshot>{
    private:
        const container::TransactionManager<avl_tree2pam_container> &m_tm;
        container::ReadTransaction<avl_tree2pam_container> m_transaction;
        const uint64_t m_num_vertices;
        const uint64_t m_num_edges;
#ifdef ENABLE_FLAT_SNAPSHOT
        std::vector<EdgeIndex> flat_snapshot;
#else
        pam_map<VertexEntryTemplate<EdgeIndexTemplate, EdgeEntry>, avl_tree> vertex_root;
#endif
    public:
        explicit Snapshot(const container::TransactionManager<avl_tree2pam_container> &tm) : m_tm(tm),
                                                                             m_transaction(tm.get_read_transaction()),
                                                                             m_num_vertices(
                                                                                     m_tm.container_impl->vertex_count()),
                                                                             m_num_edges(
                                                                                     m_tm.container_impl->edge_count())
        {
#ifdef ENABLE_FLAT_SNAPSHOT
            m_tm.container_impl->vertex_index.get_neighbor_ptrs(flat_snapshot);
#else
            vertex_root = m_transaction.container_impl->vertex_index.m_vertex_index;
#endif
        }

        Snapshot(const Snapshot &other) = default;

        Snapshot &operator=(const Snapshot &it) = delete;

        ~Snapshot() {
            m_transaction.commit();
        }

        // auto clone() const {
        //     // Snapshot new_snapshot = Snapshot(*this);
        //     // return std::make_unique<Snapshot>(std::move(new_snapshot));
            
        //     return std::make_shared<Snapshot>(*this);
        // }
        const std::shared_ptr<Snapshot> clone()  {
            // Snapshot new_snapshot = Snapshot(*this);
            // return std::make_unique<Snapshot>(std::move(new_snapshot));
            // return std::make_shared<Snapshot>(*this);
            return shared_from_this();
        }

        uint64_t size() const;

        uint64_t physical2logical(uint64_t physical) const;

        uint64_t logical2physical(uint64_t logical) const;

        uint64_t degree(uint64_t, bool logical = false) const;

        bool has_vertex(uint64_t vertex) const;

        bool has_edge(weightedEdge edge) const;

        bool has_edge(uint64_t source, uint64_t destination) const;

        uint64_t intersect(uint64_t vtx_a, uint64_t vtx_b) const;

        bool has_edge(uint64_t source, uint64_t destination, double weight) const;

        double get_weight(uint64_t source, uint64_t destination) const;

        uint64_t vertex_count() const;

        uint64_t edge_count() const;

        void get_neighbor_addr(uint64_t index) const;
        
        auto begin(uint64_t src) const;

        void edges(uint64_t index, std::vector<uint64_t> &, bool logical) const;

        template<typename F>
        uint64_t edges(uint64_t index, F&& callback, bool logical) const;
    };

    std::unique_ptr<Snapshot> get_unique_snapshot() const;

    std::shared_ptr<Snapshot> get_shared_snapshot() const;

    // Functions for debug purpose
    static std::string repl() {
        return std::string{"AVLTree_PAMTree_Wrapper"};
    }
};