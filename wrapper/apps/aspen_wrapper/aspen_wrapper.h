#pragma once

#include "libraries/aspen/code/graph/api.h"

#include "types.hpp"

#include "wrapper.h"
#include "driver.h"

class AspenWrapper{
protected:
    versioned_graph<treeplus_graph> m_graph;
    const bool m_is_directed;
    const bool m_is_weighted;
    uint64_t m_num_vertices;
private:
    struct fetch {
        fetch() {}
        inline bool update (unsigned int s, unsigned int d) const {
            return true;
        }
        inline bool updateAtomic (unsigned int s, unsigned int d) const {
            return true;
        }
        inline bool cond (unsigned int d) const {
            return true;
        }
    };
public:
    // Constructor
    explicit AspenWrapper(bool is_directed = false, bool is_weighted = false) : m_graph(empty_treeplus_graph()), m_is_directed(is_directed), m_is_weighted(is_weighted) {}
    AspenWrapper(const AspenWrapper&) = delete;
    AspenWrapper& operator=(const AspenWrapper&) = delete;
    ~AspenWrapper() = default;

    // Init
    std::shared_ptr<AspenWrapper> create_update_interface(std::string graph_type);

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

    // Snapshot Related
    class Snapshot : public std::enable_shared_from_this<Snapshot>{
    private:
        versioned_graph<treeplus_graph>* m_graph;
        versioned_graph<treeplus_graph>::version m_version;
        const uint64_t m_num_vertices;
        const uint64_t m_num_edges;
        bool m_is_weighted;

    public:
        Snapshot(versioned_graph<treeplus_graph>* graph, uint64_t vertex_count, uint64_t edge_count, bool is_weighted)
                : m_version(graph->acquire_version()),
                  m_graph(graph),
                  m_num_vertices(vertex_count),
                  m_num_edges(edge_count),
                  m_is_weighted(is_weighted)
        {}
        Snapshot(const Snapshot& other) = delete;
        Snapshot& operator=(const Snapshot& it) = delete;
        ~Snapshot() {m_graph->release_version(std::move(m_version));}

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

        void get_neighbor_addr(uint64_t index) const;

        uint64_t vertex_count() const;
        uint64_t edge_count() const;

        void edges(uint64_t vertex, std::vector<uint64_t>&, bool logical) const;

        template<typename F>
        void edges(uint64_t vertex, F&& callback, bool logical) const;
    };

    std::unique_ptr<Snapshot> get_unique_snapshot() const;
    std::shared_ptr<Snapshot> get_shared_snapshot() const;

    // Functions for test purpose
    static std::string repl() {
        return std::string{"AspenWrapper"};
    }
};
