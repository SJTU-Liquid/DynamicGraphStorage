#include "teseo_wrapper.h"

// Functions for debug purpose
namespace wrapper {
    void wrapper_test() {
        auto wrapper = TeseoWrapper();
        std::cout << "This is " << wrapper_repl(wrapper) << "!" << std::endl;
        insert_vertex(wrapper, 0);
        insert_vertex(wrapper, 1);
        insert_vertex(wrapper, 3);
        insert_edge(wrapper, 0, 1);
        insert_edge(wrapper, 0, 3);
        assert(vertex_count(wrapper) == 3);
        assert(is_empty(wrapper) == false);
        assert(has_vertex(wrapper, 0) == true);
        assert(degree(wrapper, 0) == 2);
        assert(has_edge(wrapper, 0, 1) == true);
        std::cout << wrapper_repl(wrapper) << " basic tests passed!" << std::endl;
        auto unique_snapshot = get_unique_snapshot(wrapper);
        assert(snapshot_vertex_count(unique_snapshot) == 3);
        std::cout << wrapper_repl(wrapper) << " snapshot tests passed!" << std::endl;
        std::cout << wrapper_repl(wrapper) << " tests passed!" << std::endl;
    }

    void execute(const DriverConfig & config) {
        auto wrapper = TeseoWrapper();
        Driver<TeseoWrapper, std::shared_ptr<TeseoWrapper::Snapshot>> d(wrapper, config);
        d.execute(config.workload_type, config.target_stream_type);
    }
}

#include "driver_main.h"

// Function Implemenations

std::shared_ptr<TeseoWrapper> TeseoWrapper::create_update_interface(std::string graph_type) {
    if (graph_type == "teseo") {
        return std::make_shared<TeseoWrapper>();
    }
    else {
        throw std::runtime_error("Graph name not recognized, interfaces::create_update_interface");
    }
}

// Graph Operations
void TeseoWrapper::set_max_threads(int max_threads) {}
void TeseoWrapper::init_thread(int thread_id) {
    static_cast<teseo::Teseo*>(m_pImpl)->register_thread();
}
void TeseoWrapper::end_thread(int thread_id) {
    static_cast<teseo::Teseo*>(m_pImpl)->unregister_thread();
}

bool TeseoWrapper::is_directed() const {
    return m_is_directed;
}

bool TeseoWrapper::is_weighted() const {
    return m_is_weighted;
}

bool TeseoWrapper::is_empty() const {
    return vertex_count() == 0;
}

bool TeseoWrapper::has_vertex(uint64_t vertex) const {
    auto tx = static_cast<teseo::Teseo*>(m_pImpl)->start_transaction();
    return tx.has_vertex(vertex);
}

bool TeseoWrapper::has_edge(weightedEdge edge) const {
    return this->has_edge(edge.source, edge.destination);
}

bool TeseoWrapper::has_edge(uint64_t source, uint64_t destination) const {
    auto tx = static_cast<teseo::Teseo*>(m_pImpl)->start_transaction();
    return tx.has_edge(source, destination);
}

bool TeseoWrapper::has_edge(uint64_t source, uint64_t destination, double weight) const {
    throw driver::error::FunctionNotImplementedError("teseo::has_edge(uint64_t source, uint64_t destination, double weight)");
}

uint64_t TeseoWrapper::degree(uint64_t vertex) const {
    auto tx = static_cast<teseo::Teseo*>(m_pImpl)->start_transaction();
    return tx.degree(vertex, false);
}

double TeseoWrapper::get_weight(uint64_t source, uint64_t destination) const {
    auto tx = static_cast<teseo::Teseo*>(m_pImpl)->start_transaction();
    try {
        return tx.get_weight(source, destination);
    }
    catch (const std::exception& e) {
        throw driver::error::GraphLogicalError("teseo::get_weight(uint64_t source, uint64_t destination)");
    }
}

uint64_t TeseoWrapper::logical2physical(uint64_t logical) const {
    throw driver::error::FunctionNotImplementedError("teseo::logical2physical()");
}

uint64_t TeseoWrapper::physical2logical(uint64_t physical) const {
    throw driver::error::FunctionNotImplementedError("teseo::physical2logical()");
}

uint64_t TeseoWrapper::vertex_count() const {
    auto tx = static_cast<teseo::Teseo*>(m_pImpl)->start_transaction();
    return tx.num_vertices();
}

uint64_t TeseoWrapper::edge_count() const {
    auto tx = static_cast<teseo::Teseo*>(m_pImpl)->start_transaction();
    return tx.num_edges();
}

void TeseoWrapper::get_neighbors(uint64_t vertex, std::vector<uint64_t> &neighbors) const {
    auto push_back = [&neighbors](uint64_t destination) {
        neighbors.push_back(destination);
    };
    auto iter = static_cast<teseo::Teseo*>(m_pImpl)->start_transaction().iterator();
    iter.edges(vertex, false, push_back);
}

void TeseoWrapper::get_neighbors(uint64_t vertex, std::vector<std::pair<uint64_t, double>> &neighbors) const {
    auto push_back = [&neighbors](uint64_t destination, double weight) {
        neighbors.emplace_back(destination, weight);
    };
    auto iter = static_cast<teseo::Teseo*>(m_pImpl)->start_transaction().iterator();
    iter.edges(vertex, false, push_back);
}

bool TeseoWrapper::insert_vertex(uint64_t vertex) {
    try{
        auto tx = static_cast<teseo::Teseo*>(m_pImpl)->start_transaction();
        tx.insert_vertex(vertex);
        tx.commit();
    } catch (const std::exception& e) {
        return false;
    }
    return true;
}

bool TeseoWrapper::insert_edge(uint64_t source, uint64_t destination, double weight) {
    try {
        auto tx = static_cast<teseo::Teseo*>(m_pImpl)->start_transaction();
        tx.insert_edge(source, destination, weight);
        tx.commit();
    } catch (const std::exception& e) {
        return false;
    }
    return true;
}

bool TeseoWrapper::insert_edge(uint64_t source, uint64_t destination) {
    double weight = 1.0;
    try {
        auto tx = static_cast<teseo::Teseo*>(m_pImpl)->start_transaction();
        tx.insert_edge(source, destination, weight);
        tx.commit();
    } catch (const std::exception& e) {
        return false;
    }
    return true;
}

bool TeseoWrapper::remove_vertex(uint64_t vertex) {
    try {
        auto tx = static_cast<teseo::Teseo*>(m_pImpl)->start_transaction();
        tx.remove_vertex(vertex);
        tx.commit();
    } catch (const std::exception& e) {
        return false;
    }
    return true;
}

bool TeseoWrapper::remove_edge(uint64_t source, uint64_t destination) {
    try {
        auto tx = static_cast<teseo::Teseo*>(m_pImpl)->start_transaction();
        tx.remove_edge(source, destination);
        tx.commit();
    } catch (const std::exception& e) {
        return false;
    }
    return true;
}

bool TeseoWrapper::run_batch_vertex_update(std::vector<uint64_t> &vertices, uint64_t start, uint64_t end) {
    try {
        auto tx = static_cast<teseo::Teseo*>(m_pImpl)->start_transaction();
        for (int i = start; i < end; i++) {
            tx.insert_vertex(vertices[i]);
        }
        tx.commit();
    } catch (const std::exception& e) {
        return false;
    }
    return true;
}

bool TeseoWrapper::run_batch_edge_update(std::vector<operation> & edges, uint64_t start, uint64_t end, operationType type) {
    switch (type) {
        case operationType::INSERT:
            try {
                auto tx = static_cast<teseo::Teseo*>(m_pImpl)->start_transaction();
                for (uint64_t i = start; i < end; i++) {
                    tx.insert_edge(edges[i].e.source, edges[i].e.destination, edges[i].e.weight);
                }
                tx.commit();
            } catch (const std::exception& e) {
                return false;
            }
            break;
        case operationType::DELETE:
            try {
                auto tx = static_cast<teseo::Teseo*>(m_pImpl)->start_transaction();
                for (uint64_t i = start; i < end; i++) {
                    tx.remove_edge(edges[i].e.source, edges[i].e.destination);
                }
                tx.commit();
            } catch (const std::exception& e) {
                return false;
            }
            break;
        default:
            throw driver::error::FunctionNotImplementedError("wrong update type in teseo_driver::run_batch_edge_update");
    }
    return true;
}

void TeseoWrapper::clear() {}

// Snapshot Related Function Implementations
std::unique_ptr<TeseoWrapper::Snapshot> TeseoWrapper::get_unique_snapshot() const {
    return std::make_unique<Snapshot>(m_pImpl);
}

std::shared_ptr<TeseoWrapper::Snapshot> TeseoWrapper::get_shared_snapshot() const {
    return std::make_shared<Snapshot>(m_pImpl);
}


uint64_t TeseoWrapper::Snapshot::size() const {
    return this->graph_size;
}

uint64_t TeseoWrapper::Snapshot::physical2logical(uint64_t physical) const {
    return this->m_transaction.vertex_id(physical);
}

uint64_t TeseoWrapper::Snapshot::logical2physical(uint64_t logical) const {
    return this->m_transaction.logical_id(logical);
}

uint64_t TeseoWrapper::Snapshot::degree(uint64_t vertex, bool logical) const {
    return this->m_transaction.degree(vertex, !logical);
}

bool TeseoWrapper::Snapshot::has_vertex(uint64_t vertex) const {
    return this->m_transaction.has_vertex(vertex);
}

bool TeseoWrapper::Snapshot::has_edge(weightedEdge edge) const {
    return this->m_transaction.has_edge(edge.source, edge.destination);
}

bool TeseoWrapper::Snapshot::has_edge(uint64_t source, uint64_t destination) const {
    return this->m_transaction.has_edge(source, destination);
}

bool TeseoWrapper::Snapshot::has_edge(uint64_t source, uint64_t destination, double weight) const {
    throw driver::error::FunctionNotImplementedError("teseo::has_edge(uint64_t source, uint64_t destination, double weight)");
}

double TeseoWrapper::Snapshot::get_weight(uint64_t source, uint64_t destination) const {
    try {
        return this->m_transaction.get_weight(source, destination);
    }
    catch (const std::exception& e) {
        throw driver::error::GraphLogicalError("teseo::get_weight(uint64_t source, uint64_t destination)");
    }
}

uint64_t TeseoWrapper::Snapshot::vertex_count() const {
    return this->m_transaction.num_vertices();
}

uint64_t TeseoWrapper::Snapshot::edge_count() const {
    return this->m_transaction.num_edges();
}

void TeseoWrapper::Snapshot::get_neighbor_addr(uint64_t vertex) const {
    this->m_transaction.degree(vertex, false);
}

void TeseoWrapper::Snapshot::edges(uint64_t index, std::vector<uint64_t> &neighbors, bool logical) const {
    neighbors.clear();
    neighbors.reserve(this->m_transaction.degree(index, !logical));

    auto push_back = [&neighbors](uint64_t destination) {
        neighbors.push_back(destination);
    };

    this->m_iterator.edges(index, !logical, push_back);
}

template<typename F>
void TeseoWrapper::Snapshot::edges(uint64_t index, F&& callback, bool logical) const {
    this->m_iterator.edges(index, !logical, callback);
}

// RegisterThread Related Function Implementations

// Teseo address
::teseo::Teseo* TeseoWrapper::RegisterThread::teseo() const {
    return reinterpret_cast<::teseo::Teseo*>(m_encoded_addr & ~0x1ull);
}

// Check whether to unregister the thread
bool TeseoWrapper::RegisterThread::is_enabled() const {
    return m_encoded_addr & 0x1ull;
}

// Constructor, register the thread to Teseo
TeseoWrapper::RegisterThread::RegisterThread(::teseo::Teseo *teseo) : m_encoded_addr(reinterpret_cast<uint64_t>(teseo)) {
    assert(teseo != nullptr);
    assert(teseo == this->teseo() && "Cannot decode the address of teseo");

    teseo->register_thread();
    assert(!is_enabled() && "Invalid address");
    m_encoded_addr |= 0x1ull;
    assert(is_enabled() && "That's the whole purpose of the encoding");
}

// Copy constructor
TeseoWrapper::RegisterThread::RegisterThread(const RegisterThread& rt) : m_encoded_addr(reinterpret_cast<uint64_t>(rt.teseo())){
    teseo()->register_thread();
    m_encoded_addr |= 0x1ull;
}

// Destructor, unregister the thread
TeseoWrapper::RegisterThread::~RegisterThread() {
    if(is_enabled()){
        teseo()->unregister_thread();
    }
}

// Force OpenMP to perform RAII and do not remove the call to the copy ctor.
void TeseoWrapper::RegisterThread::nop() const {}