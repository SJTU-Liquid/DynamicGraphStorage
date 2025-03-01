#pragma once

#include <tbb/concurrent_hash_map.h>

#include "libraries/LiveGraph/bind/livegraph.hpp"
#include "libraries/LiveGraph/core/transaction.hpp"
#include "third-party/libcuckoo/cuckoohash_map.hh"
#include "types.hpp"

#include "wrapper.h"
#include "driver.h"

using vertex_dictionary_t = tbb::concurrent_hash_map<uint64_t, lg::vertex_t>;
#define VertexDictionary reinterpret_cast<vertex_dictionary_t*>(m_pHashMap)

class LiveGraphWrapper{
protected:
    void *m_pImpl;
    void *m_pHashMap;
    const bool m_is_directed;
    const bool m_is_weighted;
    std::atomic<uint64_t> m_vertex_count {0};
    std::atomic<uint64_t> m_edge_count {0};
public:
    // Constructor
    explicit LiveGraphWrapper(bool is_directed = false, bool is_weighted = true) : m_is_directed(is_directed), m_is_weighted(is_weighted) {
        m_pImpl = new lg::Graph();
        m_pHashMap = new tbb::concurrent_hash_map<uint64_t, uint64_t>();
    }
    LiveGraphWrapper(const LiveGraphWrapper&) = delete;
    LiveGraphWrapper& operator=(const LiveGraphWrapper&) = delete;
    ~LiveGraphWrapper() {
        delete reinterpret_cast<lg::Graph*>(m_pImpl);
        m_pImpl = nullptr;
        delete reinterpret_cast<vertex_dictionary_t*>(m_pHashMap);
        m_pHashMap = nullptr;
    }

    // Init
    std::shared_ptr<LiveGraphWrapper> create_update_interface(std::string graph_type);

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
    class Snapshot : public std::enable_shared_from_this<Snapshot>{
    private:
        lg::Transaction m_transaction;
        void * m_pHashMap;
        const uint64_t m_num_vertices;
        const uint64_t m_num_edges;
        bool m_is_weighted;

    public:
        Snapshot(void * pImpl, void * pHashMap, uint64_t vertex_count, uint64_t edge_count, bool m_is_weighted)
                : m_transaction(reinterpret_cast<lg::Graph*>(pImpl)->begin_read_only_transaction()),
                  m_pHashMap(pHashMap),
                  m_num_vertices(vertex_count),
                  m_num_edges(edge_count),
                  m_is_weighted(m_is_weighted)
        {}
        Snapshot(const Snapshot& other) = delete;
        // Snapshot& operator=(const Snapshot& it) = delete;
        ~Snapshot() = default;
        std::shared_ptr<Snapshot> clone() const {
            return const_cast<Snapshot*>(this)->shared_from_this();
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
    };

    std::unique_ptr<Snapshot> get_unique_snapshot() const;
    std::shared_ptr<Snapshot> get_shared_snapshot() const;

    // Functions for test purpose
    static std::string repl() {
        return std::string{"LiveGraphWrapper"};
    }
};
