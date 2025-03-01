#pragma once
#include <iostream>
#include <optional>

#include "data-structure/TransactionManager.h"
#include "data-structure/VersioningBlockedSkipListAdjacencyList.h"
#include "data-structure/VersionedBlockedPropertyEdgeIterator.h"
#include "libraries/sortledton/data-structure/VersionedBlockedEdgeIterator.h"
#include "utils/commandLineParser.hpp"
#include "types.hpp"

#include "wrapper.h"
#include "driver.h"

class SortledtonWrapper {
protected:
    TransactionManager tm;
    VersioningBlockedSkipListAdjacencyList *ds;
    bool m_is_directed;
    bool m_is_weighted;
public:
    // Constructor
    SortledtonWrapper(bool is_directed = false, bool is_weighted = true, size_t property_size = 8, int block_size = 256) : tm(1), m_is_directed(is_directed), m_is_weighted(is_weighted) {
        this->ds = new VersioningBlockedSkipListAdjacencyList(block_size, property_size, tm);
    }
    SortledtonWrapper(const SortledtonWrapper&) = delete;
    SortledtonWrapper& operator=(const SortledtonWrapper&) = delete;
    ~SortledtonWrapper() {
        delete ds;
        ds = nullptr;
    }

    // Init
    std::shared_ptr<SortledtonWrapper> create_update_interface(std::string graph_type);

    // Multi-thread
    void set_max_threads(int max_threads);
    void init_thread(int thread_id);
    void end_thread(int thread_id);

    // Graph operations
    bool is_directed() const;
    bool is_weighted() const;
    bool is_empty() const;
    bool has_vertex(uint64_t vertex) const;
    bool has_edge(weightedEdge edge) const;
    bool has_edge(uint64_t source, uint64_t destination) const;
    bool has_edge(uint64_t source, uint64_t destination, double weight) const;
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
    bool run_batch_vertex_update(std::vector<uint64_t> & vertices, uint64_t start, uint64_t end);
    bool run_batch_edge_update(std::vector<operation> & edges, uint64_t start, uint64_t end, operationType type);
    void clear();

    // Snapshot Related
    class Snapshot {
    private:
        TransactionManager& m_tm;
        SnapshotTransaction m_transaction;
        const uint64_t graph_size;
        bool weighted;
    public:
        Snapshot(TransactionManager &tm, VersionedTopologyInterface *ds, uint64_t count)
                : m_tm(tm),
                  m_transaction(&m_tm, false, ds),
                  weighted(true),
                  graph_size(count) {
        }
        Snapshot(const Snapshot& other) = default;
        Snapshot& operator=(const Snapshot& it) = delete;
        ~Snapshot() {
            m_tm.transactionCompleted(m_transaction);
        }
        auto clone() const {
            Snapshot new_snapshot = Snapshot(*this);
            return std::make_shared<Snapshot>(std::move(new_snapshot));
        }

        uint64_t size() const;
        uint64_t physical2logical(uint64_t physical) const;
        uint64_t logical2physical(uint64_t logical) const;

        uint64_t degree(uint64_t, bool logical = false) const;
        bool has_vertex(uint64_t vertex) const;
        bool has_edge(weightedEdge edge) const;
        bool has_edge(uint64_t source, uint64_t destination) const;
        bool has_edge(uint64_t source, uint64_t destination, double weight) const;
        uint64_t intersect(uint64_t vtx_a, uint64_t vtx_b) const {return 0;}
        double get_weight(uint64_t source, uint64_t destination) const;

        uint64_t vertex_count() const;
        uint64_t edge_count() const;

        void get_neighbor_addr(uint64_t index) const;

        void edges(uint64_t index, std::vector<uint64_t>&, bool logical) const;

        template<typename F>
        void edges(uint64_t index, F&& callback, bool logical) const;

        auto begin(uint64_t src) {
            throw std::runtime_error("begin method is not implemented.");
        }
    };

    std::unique_ptr<Snapshot> get_unique_snapshot() const;
    std::shared_ptr<Snapshot> get_shared_snapshot() const;

    // Functions for debug purpose
    static std::string repl() {
        return std::string{"SortledtonWrapper"};
    }
};