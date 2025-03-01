#pragma once
#include <iostream>
#include <functional>
#include <optional>

#include "graph/edge.hpp"
#include "graph/edgeStream.hpp"
#include "libraries/llama/llama/include/llama.h"
#include "types.hpp"

#include "wrapper.h"
#include "driver.h"

class LLAMAWrapper{
protected:
    ll_database* m_database;
    const bool m_is_directed;
    const bool m_is_weighted;
    
private:
    // NOTE: This function will make the program crash when there is no vertex inserted.
    // That's because in this case the result of num_levels() is 0, which makes the
    // failure in first assert in the constructor of ll_mlcsr_ro_graph :(
    ll_mlcsr_ro_graph* get_llama_snapshot(ll_database *g) const {
        int numl = g->graph()->ro_graph().num_levels();
        return new ll_mlcsr_ro_graph{ & (g->graph()->ro_graph()) , numl - 1};
    }
public:
    // Constructor
    explicit LLAMAWrapper(bool is_directed = false, bool is_weighted = true) : m_is_directed(is_directed), m_is_weighted(is_weighted) {
        char* database_directory = (char*) alloca(16);
        strcpy(database_directory, "db");
        m_database = new ll_database(database_directory);

        char const * const g_llama_property_weights = "weights";
        auto& csr = m_database->graph()->ro_graph();
        csr.create_uninitialized_edge_property_64(g_llama_property_weights, LL_T_DOUBLE);
    }
    LLAMAWrapper(const LLAMAWrapper&) = delete;
    LLAMAWrapper& operator=(const LLAMAWrapper&) = delete;
    ~LLAMAWrapper() {
        if (m_database != nullptr) {
            free(m_database);
        }
    }

    // Init
    void load(const std::string& path, driver::reader::readerType type);
    std::shared_ptr<LLAMAWrapper> create_update_interface(std::string graph_type);

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
        ll_mlcsr_ro_graph* m_snapshot;
        bool m_is_directed;
        bool m_is_weighted;

    public:
        Snapshot(ll_mlcsr_ro_graph* snapshot, bool is_directed, bool is_weighted)
                : m_snapshot(snapshot), m_is_directed(is_directed), m_is_weighted(is_weighted) {}
        Snapshot(const Snapshot& other) = delete;
        Snapshot& operator=(const Snapshot& it) = delete;
        ~Snapshot() {
            if (m_snapshot != nullptr) {
                free(m_snapshot);
            }
        }
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
        uint64_t intersect(uint64_t vtx_a, uint64_t vtx_b) const {}
        double get_weight(uint64_t source, uint64_t destination) const;

        uint64_t vertex_count() const;
        uint64_t edge_count() const;

        void get_neighbor_addr(uint64_t index) const;

        void edges(uint64_t vertex, std::vector<uint64_t>&, bool logical) const;
        void edges(uint64_t vertex, std::function<void(uint64_t, double)> callback, bool logical) const;
    };

    std::unique_ptr<Snapshot> get_unique_snapshot() const;
    std::shared_ptr<Snapshot> get_shared_snapshot() const;

    // Functions for test purpose
    static std::string repl() {
        return std::string{"LLAMAWrapper"};
    }
};