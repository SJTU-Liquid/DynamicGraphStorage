#include "sortledton_wrapper.h"

// Functions for debug purpose
namespace wrapper {
    void wrapper_test() {
        auto wrapper = SortledtonWrapper();
        std::cout << "This is " << wrapper_repl(wrapper) << "!" << std::endl;
        insert_vertex(wrapper, 0);
        insert_vertex(wrapper, 1);
        insert_vertex(wrapper, 2);
        insert_vertex(wrapper, 3);
        insert_edge(wrapper, 0, 1);
        insert_edge(wrapper, 0, 2);
        assert(vertex_count(wrapper) == 4);
        assert(is_empty(wrapper) == false);
        assert(has_vertex(wrapper, 0) == true);
        assert(degree(wrapper, 0) == 2);
        assert(has_edge(wrapper, 0, 1) == true);
        std::cout << wrapper_repl(wrapper) << " basic tests passed!" << std::endl;
        auto unique_snapshot = get_unique_snapshot(wrapper);
        auto cloned_snapshot = snapshot_clone(unique_snapshot);
        assert(snapshot_vertex_count(unique_snapshot) == 4);
        assert(snapshot_vertex_count(cloned_snapshot) == 4);
        std::cout << wrapper_repl(wrapper) << " snapshot tests passed!" << std::endl;
        std::cout << wrapper_repl(wrapper) << " tests passed!" << std::endl;
        std::cout << "Note: We have found that a assertion failure might occur in the destructure of the system. It's a problem of the system itself, not the wrapper." << std::endl;
    }

    void execute(const DriverConfig & config) {
        auto wrapper = SortledtonWrapper(false, true);
        Driver<SortledtonWrapper, std::shared_ptr<SortledtonWrapper::Snapshot>> d(wrapper, config);
        d.execute(config.workload_type, config.target_stream_type);
    }
}

#include "driver_main.h"

// Function Implementations


std::shared_ptr<SortledtonWrapper> SortledtonWrapper::create_update_interface(std::string graph_type) {
    if (graph_type == "sortledton") {
        return std::make_shared<SortledtonWrapper>();
    }
    else {
        throw std::runtime_error("Graph name not recognized, interfaces::create_update_interface");
    }
}

// Multi-thread
void SortledtonWrapper::set_max_threads(int max_threads) {
    tm.reset_max_threads(max_threads);
}
void SortledtonWrapper::init_thread(int thread_id) {
    tm.register_thread(thread_id);
}
void SortledtonWrapper::end_thread(int thread_id) {
    tm.deregister_thread(thread_id);
}

// Graph Operations
bool SortledtonWrapper::is_directed() const {
    return m_is_directed;
}

bool SortledtonWrapper::is_weighted() const {
    return m_is_weighted;
}

bool SortledtonWrapper::is_empty() const {
    return (this->vertex_count() == 0);
}

bool SortledtonWrapper::has_vertex(uint64_t vertex) const {
    auto *non_const_this = const_cast<SortledtonWrapper *>(this);
    SnapshotTransaction tx = non_const_this->tm.getSnapshotTransaction(ds, false);
    auto has_vertex = tx.has_vertex(vertex);
    non_const_this->tm.transactionCompleted(tx);
    return has_vertex;
}

bool SortledtonWrapper::has_edge(weightedEdge edge) const {
    return has_edge(edge.source, edge.destination, edge.weight);
}

bool SortledtonWrapper::has_edge(uint64_t source, uint64_t destination) const {
    auto *non_const_this = const_cast<SortledtonWrapper *>(this);
    SnapshotTransaction tx = non_const_this->tm.getSnapshotTransaction(ds, false);

    if (!tx.has_vertex(source) || !tx.has_vertex(destination)) {
        throw driver::error::GraphLogicalError("Vertex does not exist : sortledtonDriver::has_edge");
    }
    weight_t w;
    auto has_edge = tx.get_weight({static_cast<dst_t>(source), static_cast<dst_t>(destination)}, (char *) &w);
    non_const_this->tm.transactionCompleted(tx);
    return has_edge;
}

bool SortledtonWrapper::has_edge(uint64_t source, uint64_t destination, double weight) const {
    if (!m_is_weighted) {
        throw driver::error::GraphLogicalError("Graph is not weighted : sortledtonDriver::has_edge");
    }

    auto *non_const_this = const_cast<SortledtonWrapper *>(this);
    SnapshotTransaction tx = non_const_this->tm.getSnapshotTransaction(ds, false);

    if (!tx.has_vertex(source) || !tx.has_vertex(destination)) {
        throw driver::error::GraphLogicalError("Vertex does not exist : sortledtonDriver::has_edge");
    }
    weight_t w;
    auto has_edge = tx.get_weight({static_cast<dst_t>(source), static_cast<dst_t>(destination)}, (char *) &w);
    non_const_this->tm.transactionCompleted(tx);
    return has_edge && (w == weight);
}

uint64_t SortledtonWrapper::degree(uint64_t vertex) const {
    auto *non_const_this = const_cast<SortledtonWrapper *>(this);
    SnapshotTransaction tx = non_const_this->tm.getSnapshotTransaction(ds, false);

    if (!tx.has_vertex(vertex)) {
        throw driver::error::GraphLogicalError("Vertex does not exist : sortledtonDriver::degree");
    }
    auto degree = tx.neighbourhood_size(vertex);
    non_const_this->tm.transactionCompleted(tx);
    return degree;
}

double SortledtonWrapper::get_weight(uint64_t source, uint64_t destination) const {
    if (!m_is_weighted) {
        throw driver::error::GraphLogicalError("Graph is not m_is_weighted : sortledtonDriver::get_weight");
    }

    auto *non_const_this = const_cast<SortledtonWrapper *>(this);
    SnapshotTransaction tx = non_const_this->tm.getSnapshotTransaction(ds, false);

    if (!tx.has_vertex(source) || !tx.has_vertex(destination)) {
        throw driver::error::GraphLogicalError("Vertex does not exist : sortledtonDriver::get_weight");
    }
    weight_t w;
    auto has_edge = tx.get_weight({static_cast<dst_t>(source), static_cast<dst_t>(destination)}, (char *) &w);
    non_const_this->tm.transactionCompleted(tx);
    // return has_edge ? w : nan("");
    if (has_edge) {
        return w;
    } else {
        throw driver::error::GraphLogicalError("Edge does not exist : sortledtonDriver::get_weight");
    }
}

uint64_t SortledtonWrapper::logical2physical(uint64_t vertex) const {
    throw driver::error::FunctionNotImplementedError("logical2physical");
}

uint64_t SortledtonWrapper::physical2logical(uint64_t physical) const {
    throw driver::error::FunctionNotImplementedError("physical2logical");
}

uint64_t SortledtonWrapper::vertex_count() const {
    auto *non_const_this = const_cast<SortledtonWrapper *>(this);
    SnapshotTransaction tx = non_const_this->tm.getSnapshotTransaction(ds, false);
    auto num_vertices = tx.vertex_count();
    non_const_this->tm.transactionCompleted(tx);
    return num_vertices;
}

uint64_t SortledtonWrapper::edge_count() const {
    auto *non_const_this = const_cast<SortledtonWrapper *>(this);
    SnapshotTransaction tx = non_const_this->tm.getSnapshotTransaction(ds, false);
    auto num_edges = tx.edge_count();
    non_const_this->tm.transactionCompleted(tx);
    return num_edges;
}

void SortledtonWrapper::get_neighbors(uint64_t vertex, vector<uint64_t> &) const {
    throw driver::error::FunctionNotImplementedError("get_neighbors");
}

void SortledtonWrapper::get_neighbors(uint64_t vertex, vector<std::pair<uint64_t, double>> &) const {
    throw driver::error::FunctionNotImplementedError("get_neighbors");
}

bool SortledtonWrapper::insert_vertex(uint64_t vertex) {
    SnapshotTransaction tx = tm.getSnapshotTransaction(ds, true);
    bool inserted = true;
    try {
        tx.insert_vertex(vertex);
        tx.execute();
    } catch (exception &e) {
        inserted = false;
    }
    tm.transactionCompleted(tx);
    return inserted;
}

bool SortledtonWrapper::insert_edge(uint64_t source, uint64_t destination, double weight) {
    edge_t internal_edge(static_cast<dst_t>(source), static_cast<dst_t>(destination));

    thread_local optional <SnapshotTransaction> tx_o = tm.getSnapshotTransaction(ds, true);
    auto tx = *tx_o;

    VertexExistsPrecondition pre_v1(internal_edge.src);
    tx.register_precondition(&pre_v1);
    VertexExistsPrecondition pre_v2(internal_edge.dst);
    tx.register_precondition(&pre_v2);

    // test
    bool inserted = true;
    try {
        tx.insert_edge(internal_edge, (char *) &weight, sizeof(weight));
        if (!m_is_directed) {
            tx.insert_edge({internal_edge.dst, internal_edge.src}, (char *) &weight, sizeof(weight));
        }
        inserted = tx.execute();
    } catch (...) {
        inserted = false;
    }
    tm.transactionCompleted(tx);
    return inserted;
}

bool SortledtonWrapper::insert_edge(uint64_t source, uint64_t destination) {
    edge_t internal_edge(static_cast<dst_t>(source), static_cast<dst_t>(destination));

    thread_local optional <SnapshotTransaction> tx_o = tm.getSnapshotTransaction(ds, true);
    auto tx = *tx_o;

    VertexExistsPrecondition pre_v1(internal_edge.src);
    tx.register_precondition(&pre_v1);
    VertexExistsPrecondition pre_v2(internal_edge.dst);
    tx.register_precondition(&pre_v2);

    // test
    bool inserted = true;
    double weight = random() / ((double) RAND_MAX);
    try {
        tx.insert_edge(internal_edge, (char *) &weight, sizeof(weight));
        if (!m_is_directed) {
            tx.insert_edge({internal_edge.dst, internal_edge.src}, (char *) &weight, sizeof(weight));
        }
        inserted = tx.execute();
    } catch (...) {
        inserted = false;
    }
    tm.transactionCompleted(tx);
    return inserted;
}

bool SortledtonWrapper::remove_vertex(uint64_t vertex) {
    SnapshotTransaction tx = tm.getSnapshotTransaction(ds, true);
    bool deleted = true;
    try {
        tx.delete_vertex(vertex);
        tx.execute();
    } catch (exception &e) {
        deleted = false;
    }
    tm.transactionCompleted(tx);
    return deleted;
}

bool SortledtonWrapper::remove_edge(uint64_t source, uint64_t destination) {
    thread_local optional <SnapshotTransaction> tx_o = tm.getSnapshotTransaction(ds, true);
    auto tx = *tx_o;

    edge_t internal_edge(static_cast<dst_t>(source), static_cast<dst_t>(destination));

    if (!m_is_directed) {
        tx.delete_edge({internal_edge.dst, internal_edge.src});
    }
    tx.delete_edge(internal_edge);

    bool removed = tx.execute();

    tm.transactionCompleted(tx);
    return removed;
}

bool SortledtonWrapper::run_batch_vertex_update(vector<uint64_t> &vertices, uint64_t start, uint64_t end) {
    bool inserted = true;
    try {
        SnapshotTransaction tx = tm.getSnapshotTransaction(ds, true);
        for (int i = start; i < end; i++) {
            tx.insert_vertex(vertices[i]);
            tx.execute();
            tm.transactionCompleted(tx);
            tm.getSnapshotTransaction(ds, true, tx);
        }
    } catch (exception &e) {
        inserted = false;
    }
    return inserted;
}

bool SortledtonWrapper::run_batch_edge_update(std::vector<operation> & edges, uint64_t start, uint64_t end, operationType type) {
    switch (type) {
        case operationType::INSERT:
            try {
                thread_local optional <SnapshotTransaction> tx_o = tm.getSnapshotTransaction(ds, true);
                auto tx = *tx_o;
                double weight = random() / ((double) RAND_MAX);

                for (uint64_t i = start; i < end; i++) {
                    auto edge = edges[i].e;
                    if (m_is_weighted) weight = edge.weight;
                    edge_t e(static_cast<dst_t>(edge.source), static_cast<dst_t>(edge.destination));
                    // VertexExistsPrecondition pre_v1(e.src);
                    // tx.register_precondition(&pre_v1);
                    // VertexExistsPrecondition pre_v2(e.dst);
                    // tx.register_precondition(&pre_v2);

                    if (!m_is_directed) {
                        auto opposite = edge_t(edge.destination, edge.source);
                        tx.insert_edge(opposite, (char *) &weight, sizeof(weight));
                    }

                    tx.insert_edge({e.src, e.dst}, (char *) &weight, sizeof(weight));

                    // tx.execute();
                    // tm.transactionCompleted(tx);
                    // tm.getSnapshotTransaction(ds, true, tx);
                }
                tx.execute();
                tm.transactionCompleted(tx);
            } catch (...) {
                return false;
            }
            break;
        case operationType::DELETE:
            try {
                thread_local optional <SnapshotTransaction> tx_o = tm.getSnapshotTransaction(ds, true);
                auto tx = *tx_o;

                for (uint64_t i = start; i < end; i++) {
                    auto edge = edges[i].e;
                    edge_t e(static_cast<dst_t>(edge.source), static_cast<dst_t>(edge.destination));

                    if (!m_is_directed) {
                        auto opposite = edge_t{e.dst, e.src};
                        tx.delete_edge(opposite);
                    }
                    tx.delete_edge(e);
                    tx.execute();
                    tm.transactionCompleted(tx);
                    tm.getSnapshotTransaction(ds, true, tx);
                }
            } catch (...) {
                return false;
            }
            break;
    }
    return true;
}

void SortledtonWrapper::clear() {

}

// Snapshot Related Function Implementations
unique_ptr<SortledtonWrapper::Snapshot> SortledtonWrapper::get_unique_snapshot() const {
    auto *non_const_this = const_cast<SortledtonWrapper *>(this);
    Snapshot tmp(non_const_this->tm, non_const_this->ds, vertex_count());
    return std::make_unique<Snapshot>(std::move(tmp));
}

shared_ptr<SortledtonWrapper::Snapshot> SortledtonWrapper::get_shared_snapshot() const {
    auto *non_const_this = const_cast<SortledtonWrapper *>(this);
    uint64_t size = vertex_count();
    Snapshot tmp(non_const_this->tm, non_const_this->ds, size);
    return std::make_shared<Snapshot>(std::move(tmp));
}


uint64_t SortledtonWrapper::Snapshot::size() const {
    return this->graph_size;
}

uint64_t SortledtonWrapper::Snapshot::physical2logical(uint64_t physical) const {
    auto non_const_this = const_cast<Snapshot*>(this);
    return non_const_this->m_transaction.logical_id(physical);
}

uint64_t SortledtonWrapper::Snapshot::logical2physical(uint64_t logical) const {
    auto non_const_this = const_cast<Snapshot*>(this);
    return non_const_this->m_transaction.physical_id(logical);
}

uint64_t SortledtonWrapper::Snapshot::degree(uint64_t vertex, bool logical) const {
    auto non_const_this = const_cast<Snapshot*>(this);
    if (!logical) {
        vertex = non_const_this->m_transaction.logical_id(vertex);
    }
    if (!has_vertex(vertex)) {
        throw driver::error::GraphLogicalError("Vertex does not exist : sortledtonDriver::snapshot::degree");
    }
    
    return non_const_this->m_transaction.neighbourhood_size(vertex);
}

bool SortledtonWrapper::Snapshot::has_vertex(uint64_t vertex) const {
    auto non_const_this = const_cast<Snapshot*>(this);
    return non_const_this->m_transaction.has_vertex(vertex);
}

bool SortledtonWrapper::Snapshot::has_edge(weightedEdge edge) const {
    if (weighted) {
        return has_edge(edge.source, edge.destination, edge.weight);
    }
    else {
        return has_edge(edge.source, edge.destination);
    }
}

bool SortledtonWrapper::Snapshot::has_edge(uint64_t source, uint64_t destination) const {
    if (!has_vertex(source) || !has_vertex(destination)) {
        throw driver::error::GraphLogicalError("Vertex does not exist : sortledtonDriver::has_edge");
    }
    weight_t w;
    auto non_const_this = const_cast<Snapshot*>(this);
    auto has_edge = non_const_this->m_transaction.get_weight({static_cast<dst_t>(source), static_cast<dst_t>(destination)}, (char *) &w);
    return has_edge;
}

bool SortledtonWrapper::Snapshot::has_edge(uint64_t source, uint64_t destination, double weight) const {
    if (!weighted) {
        throw driver::error::GraphLogicalError("Graph is not m_is_weighted : sortledtonDriver::has_edge");
    }
    if (!has_vertex(source) || !has_vertex(destination)) {
        throw driver::error::GraphLogicalError("Vertex does not exist : sortledtonDriver::has_edge");
    }
    auto non_const_this = const_cast<Snapshot*>(this);
    weight_t w;
    auto has_edge = non_const_this->m_transaction.get_weight({static_cast<dst_t>(source), static_cast<dst_t>(destination)}, (char *) &w);

    return has_edge && (w == weight);
}

double SortledtonWrapper::Snapshot::get_weight(uint64_t source, uint64_t destination) const {
    if (!weighted) {
        throw driver::error::GraphLogicalError("Graph is not m_is_weighted : sortledtonDriver::get_weight");
    }

    if (!has_vertex(source) || !has_vertex(destination)) {
        throw driver::error::GraphLogicalError("Vertex does not exist : sortledtonDriver::get_weight");
    }

    weight_t w;
    auto non_const_this = const_cast<Snapshot*>(this);
    auto has_edge = non_const_this->m_transaction.get_weight({static_cast<dst_t>(source), static_cast<dst_t>(destination)}, (char *) &w);

    if (has_edge) {
        return w;
    } else {
        throw driver::error::GraphLogicalError("Edge does not exist : sortledtonDriver::get_weight");
    }
}

uint64_t SortledtonWrapper::Snapshot::vertex_count() const {
    auto non_const_this = const_cast<Snapshot*>(this);
    return non_const_this->m_transaction.vertex_count();
}

uint64_t SortledtonWrapper::Snapshot::edge_count() const {
    auto non_const_this = const_cast<Snapshot*>(this);
    return non_const_this->m_transaction.edge_count();
}

void SortledtonWrapper::Snapshot::get_neighbor_addr(uint64_t index) const {
    auto non_const_this = const_cast<Snapshot*>(this);
    VersionedBlockedEdgeIterator _iter = non_const_this->m_transaction.neighbourhood_blocked_p(index); 
}

void SortledtonWrapper::Snapshot::edges(uint64_t index, vector<uint64_t> &neighbors, bool logical) const {
    auto non_const_this = const_cast<Snapshot*>(this);

    if (logical) {
        index = non_const_this->m_transaction.physical_id(index);
    }

    SORTLEDTON_ITERATE(non_const_this->m_transaction, index, {
        if (logical) {
            e = physical2logical(e);
        }
        neighbors.push_back(e);
    });
    std::sort(neighbors.begin(), neighbors.end());
}

template<typename F>
void SortledtonWrapper::Snapshot::edges(uint64_t index, F&& callback, bool logical) const {
    auto non_const_this = const_cast<Snapshot*>(this);

    if (logical) {
        index = non_const_this->m_transaction.physical_id(index);
    }
    vertexID e;
    double w;
    SORTLEDTON_ITERATE_WITH_PROPERTIES_NAMED(non_const_this->m_transaction, index, e, w, end_iteration, {
        if (logical) {
            e = physical2logical(e);
        }
        
        callback(e, w);
    });
    
}
