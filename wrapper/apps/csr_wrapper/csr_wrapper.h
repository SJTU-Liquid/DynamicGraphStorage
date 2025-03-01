#pragma once

#include "types.hpp"

#include "wrapper.h"
#include "driver.h"

#include <algorithm>
#include <cassert>

class CsrWrapper{
protected:
    std::vector<vertexID> row_ptr;
    std::vector<vertexID> col_ind;
    std::vector<double> weight;

    const bool m_is_directed;
    const bool m_is_weighted;
    uint64_t m_num_vertices;

public:
    // Constructor
    explicit CsrWrapper(bool is_directed = false, bool is_weighted = true) : m_is_directed(is_directed), m_is_weighted(is_weighted) {}

    CsrWrapper(const CsrWrapper&) = delete;
    CsrWrapper& operator=(const CsrWrapper&) = delete;
    ~CsrWrapper() = default;

    // Init
    void load(const std::string& row_path, std::string& col_path, std::string& weight_path);
    std::shared_ptr<CsrWrapper> create_update_interface(std::string graph_type);

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

    // Graph operations
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

    struct CSRIterator;
    // Snapshot Related
    class Snapshot : public std::enable_shared_from_this<Snapshot> {
    private:
        const CsrWrapper &m_graph;

    public:
        Snapshot(const CsrWrapper &graph) : m_graph(graph) {}
        Snapshot(const Snapshot& other) = delete;
        Snapshot& operator=(const Snapshot& it) = delete;
        ~Snapshot() {}
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
        uint64_t intersect(uint64_t vtx_a, uint64_t vtx_b) const;
        double get_weight(uint64_t source, uint64_t destination) const;

        uint64_t get_neighbor_addr(uint64_t index) const;

        uint64_t vertex_count() const;
        uint64_t edge_count() const;

        void edges(uint64_t vertex, std::vector<uint64_t>&, bool logical) const;

        template <typename T>
        uint64_t edges(uint64_t vertex, T const & callback, bool logical) const;

        CSRIterator begin(uint64_t source) {
            return CSRIterator(m_graph, source);
        }
    };

    std::unique_ptr<Snapshot> get_unique_snapshot() const;
    std::shared_ptr<Snapshot> get_shared_snapshot() const;

    // Functions for test purpose
    static std::string repl() {
        return std::string{"CsrWrapper"};
    }

    struct CSRIterator {
        uint64_t source;
        std::vector<uint64_t>::const_iterator it;
        const CsrWrapper &m_graph;

        CSRIterator(const CsrWrapper &graph, uint64_t src) : m_graph(graph), source(src) {
            it = m_graph.col_ind.begin() + m_graph.row_ptr[source];
        }
        

        bool is_valid() {
            return (it != m_graph.col_ind.begin() + m_graph.row_ptr[source + 1]);
        }

        CSRIterator& operator++() {
            ++it;
            return (*this);
        }

        uint64_t operator*() {
            return *it;
        }
    };
};