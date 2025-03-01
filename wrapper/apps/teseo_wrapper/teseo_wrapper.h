#pragma once

#include "libraries/teseo/include/teseo.hpp"
#include "types.hpp"

#include "wrapper.h"
#include "driver.h"

class TeseoWrapper{
protected:
    void* m_pImpl;
    const bool m_is_weighted;
    const bool m_is_directed;
private:
    class RegisterThread {
        RegisterThread& operator=(const RegisterThread&) = delete;
        uint64_t m_encoded_addr; // The address of Teseo + 1 bit to check whether to unregister the thread
        ::teseo::Teseo* teseo() const;
        bool is_enabled() const;
    public:
        RegisterThread(::teseo::Teseo* teseo);
        RegisterThread(const RegisterThread& rt);
        ~RegisterThread();
        void nop() const;
    };
public:
    // Constructor
    explicit TeseoWrapper(bool is_directed = false, bool is_weighted = false): m_pImpl(new teseo::Teseo()), m_is_weighted(is_weighted), m_is_directed(is_directed) {}
    TeseoWrapper(const TeseoWrapper&) = delete;
    TeseoWrapper& operator=(const TeseoWrapper&) = delete;
    ~TeseoWrapper() {delete static_cast<teseo::Teseo*>(m_pImpl);}

    // Init
    std::shared_ptr<TeseoWrapper> create_update_interface(std::string graph_type);

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
        teseo::Transaction m_transaction;
        teseo::Iterator m_iterator;
        const uint64_t graph_size;
    public:
        Snapshot(void * teseo_impl)
                : m_transaction(static_cast<teseo::Teseo*>(teseo_impl)->start_transaction(true)),
                  graph_size(m_transaction.num_vertices()),
                  m_iterator(m_transaction.iterator())
        {}
        Snapshot(const Snapshot& other)
                : m_transaction(other.m_transaction), graph_size(other.graph_size), m_iterator(m_transaction.iterator()){}
        Snapshot& operator=(const Snapshot& it) = delete;
        ~Snapshot() = default;
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
        double get_weight(uint64_t source, uint64_t destination) const;

        uint64_t vertex_count() const;
        uint64_t edge_count() const;

        void get_neighbor_addr(uint64_t index) const;

        void edges(uint64_t index, std::vector<uint64_t>&, bool logical) const;
        uint64_t intersect(uint64_t vtx_a, uint64_t vtx_b) const {return 0;}

        template<typename F>
        void edges(uint64_t index, F&& callback, bool logical) const;
    };

    std::unique_ptr<Snapshot> get_unique_snapshot() const;
    std::shared_ptr<Snapshot> get_shared_snapshot() const;

    // Functions for debug purpose
    static std::string repl() {
        return std::string{"TeseoWrapper"};
    }
};