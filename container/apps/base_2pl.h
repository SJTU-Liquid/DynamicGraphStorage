#pragma once

#include "transaction/transaction_2pl.hpp"
#include "utils/types.hpp"
#include "utils/config.hpp"
#include "container/container_2pl.hpp"

#include "../types/types.hpp"


#include "wrapper.h"
#include "driver.h"

using namespace container;

template <typename ContainerType>
class WrapperBase {
protected:
    TransactionManager<ContainerType> tm;
    bool m_is_directed;
    bool m_is_weighted;

public:
    explicit WrapperBase(bool is_directed, bool is_weighted)
        : tm(is_directed, is_weighted), m_is_weighted(is_weighted), m_is_directed(is_directed) {}

    virtual ~WrapperBase() = default;

    bool is_directed() const { return m_is_directed; }
    bool is_weighted() const { return m_is_weighted; }
    bool is_empty() const { return vertex_count() == 0; }

    void set_max_threads(int max_threads) {};
    void init_thread(int thread_id) {};
    void end_thread(int thread_id) {};

    bool has_vertex(uint64_t vertex) const {
        auto tx = tm.get_read_transaction();
        bool result = tx.has_vertex(vertex);
        tx.commit();
        return result;
    }

    bool has_edge(uint64_t source, uint64_t destination) const {
        auto tx = tm.get_read_transaction();
        bool result = tx.has_edge(source, destination);
        tx.commit();
        return result;
    }

    uint64_t degree(uint64_t vertex) const {
        auto tx = tm.get_read_transaction();
        if (!tx.has_vertex(vertex)) {
            throw std::logic_error("Vertex does not exist");
        }
        uint64_t degree = tx.get_degree(vertex);
        tx.commit();
        return degree;
    }

    uint64_t vertex_count() const {
        auto tx = tm.get_read_transaction();
        uint64_t count = tx.container_impl->vertex_count();
        tx.commit();
        return count;
    }

    uint64_t edge_count() const {
        auto tx = tm.get_read_transaction();
        uint64_t count = tx->edge_count();
        tx.commit();
        return count;
    }

    bool insert_vertex(uint64_t vertex) {
        auto tx = tm.get_write_transaction();
        bool success = true;
        try {
            tx.insert_vertex(vertex);
            tx.commit();
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            tx.abort();
            success = false;
        }
        return success;
    }

    bool insert_edge(uint64_t source, uint64_t destination, double weight) {
        auto tx = tm.get_write_transaction();
        bool inserted = true;
        try {
            tx.insert_edge(source, destination);
            if (!m_is_directed) {
                tx.insert_edge(destination, source);
            }
            inserted = tx.commit();
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            tx.abort();
            inserted = false;
        }
        return inserted;
    }

    bool insert_edge(uint64_t source, uint64_t destination) {
        return insert_edge(source, destination, 0);
    }

    virtual bool remove_vertex(uint64_t vertex) {
        throw driver::error::FunctionNotImplementedError("remove_vertex");
    }

    virtual bool remove_edge(uint64_t source, uint64_t destination) {
        throw driver::error::FunctionNotImplementedError("remove_edge");
    }


    bool run_batch_vertex_update(std::vector<uint64_t> &vertices, uint64_t start, uint64_t end) {
        auto tx = tm.get_write_transaction();
        bool inserted = true;
        try {
            for (int i = start; i < end; i++) {
                tx.insert_vertex(vertices[i]);
            }
            inserted = tx.commit();
        } catch (std::exception &e) {
            std::cerr << e.what() << std::endl;
            tx.abort();
            inserted = false;
        }
        return inserted;
    }

    bool run_batch_edge_update(std::vector<operation> & edges, uint64_t start, uint64_t end, operationType type) {
        switch (type) {
            case operationType::INSERT: {
                auto tx = tm.get_write_transaction();
                try {
                    for (uint64_t i = start; i < end; i++) {
                        auto edge = edges[i].e;
                        tx.insert_edge(edge.source, edge.destination);
                        if (!m_is_directed) {
                            tx.insert_edge(edge.destination, edge.source);
                        }
                    }
                    tx.commit();
                } catch (std::exception &e) {
                    std::cerr << e.what() << std::endl;
                    tx.abort();
                    return false;
                }
                break;
            }
            default:
                throw driver::error::FunctionNotImplementedError("run_batch_edge_update:: except INSERT");
                break;
        }
        return true;
    }

    void clear() {

    }

    // Snapshot Related
    class Snapshot {
    private:
        const TransactionManager<ContainerType>& m_tm;
        ReadTransaction<ContainerType> m_transaction;
        const uint64_t m_num_vertices;

    public:
        explicit Snapshot(const TransactionManager<ContainerType>& tm)
            : m_tm(tm),
              m_transaction(tm.get_read_transaction()),
              m_num_vertices(m_tm.container_impl->vertex_count()) {}

        Snapshot(const Snapshot& other) = default;

        Snapshot& operator=(const Snapshot&) = delete;

        ~Snapshot() {
            m_transaction.commit();
        }

        auto clone() const {
            return std::make_shared<Snapshot>(*this);
        }

        uint64_t size() const {
            return m_transaction.edge_count();
        }

        uint64_t physical2logical(uint64_t physical) const {
            return physical;
        }

        uint64_t logical2physical(uint64_t logical) const {
            return logical;
        }

        uint64_t degree(uint64_t vertex, bool logical = false) const {
            if (!has_vertex(vertex)) {
                throw std::logic_error("Vertex does not exist");
            }
            return m_transaction.get_degree(vertex);
        }

        bool has_vertex(uint64_t vertex) const {
            auto non_const_this = const_cast<Snapshot*>(this);
            return non_const_this->m_transaction.has_vertex(vertex);
        }

        bool has_edge(uint64_t source, uint64_t destination) const {
            return m_transaction.has_edge(source, destination);
        }

        uint64_t intersect(uint64_t vtx_a, uint64_t vtx_b) const {
            return m_transaction.intersect(vtx_a, vtx_b);
        }

        uint64_t vertex_count() const {
            return this->m_num_vertices;
        }

        uint64_t edge_count() const {
            return m_transaction.edge_count();
        }

        void get_neighbors(uint64_t vertex, std::vector<uint64_t>& neighbors) const {
            m_transaction.get_neighbor(vertex, neighbors);
            std::sort(neighbors.begin(), neighbors.end());
        }

        template <typename F>
        void edges(uint64_t index, F&& callback, bool logical) const {
            m_transaction.edges(index, std::forward<F>(callback));
        }

        double get_weight(uint64_t source, uint64_t destination) const {
            throw driver::error::FunctionNotImplementedError("get_weight");
        }

        void get_neighbor_addr(uint64_t index) const {
            m_transaction.get_neighbor_ptr(index);
        }

        auto begin(uint64_t src) const {
            return m_transaction.begin(src);
        }
    };

    std::unique_ptr<Snapshot> get_unique_snapshot() const {
        Snapshot tmp(tm);
        return std::make_unique<Snapshot>(std::move(tmp));
    }

    std::shared_ptr<Snapshot> get_shared_snapshot() const {
        // Snapshot tmp(tm);
        return std::make_shared<Snapshot>(tm);
    }

    // class IteratorBase {
    //     const TransactionManager<ContainerType>& m_tm;
    //     ReadTransaction<ContainerType> m_transaction;
    // public:
    //     virtual ~IteratorBase() = default;
    //     virtual bool has_next() const {}
    //     virtual void next() {}
    //     virtual uint64_t operator*() const {}
    // };

    // virtual std::unique_ptr<IteratorBase> get_iterator(uint64_t src) const {}
};