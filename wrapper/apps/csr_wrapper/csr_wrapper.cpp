#include "csr_wrapper.h"

// Functions for debug purpose
namespace wrapper {
    void wrapper_test() {
        auto wrapper = CsrWrapper();
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
        auto wrapper = CsrWrapper();
        Driver<CsrWrapper, std::shared_ptr<CsrWrapper::Snapshot>> d(wrapper, config);
        d.execute(config.workload_type, config.target_stream_type);
    }
}

#include "driver_main.h"

// Function Implementations

std::shared_ptr<CsrWrapper> CsrWrapper::create_update_interface(std::string graph_type) {
    if (graph_type == "csr") {
        return std::make_shared<CsrWrapper>();
    }
    else {
        throw std::runtime_error("Graph name not recognized, interfaces::create_update_interface");
    }
}

// Multi-thread
void CsrWrapper::set_max_threads(int max_threads) {}
void CsrWrapper::init_thread(int thread_id) {}
void CsrWrapper::end_thread(int thread_id) {}

// Graph Operations
bool CsrWrapper::is_directed() const {
    return m_is_directed;
}

bool CsrWrapper::is_weighted() const {
    return m_is_weighted;
}

bool CsrWrapper::is_empty() const {
    return col_ind.size() == 0;
}

bool CsrWrapper::has_vertex(uint64_t vertex) const {
    return vertex < row_ptr.size() - 1;
}

bool CsrWrapper::has_edge(weightedEdge edge) const {
    if (edge.source >= row_ptr.size() - 1) {
        return false;
    }
    if (m_is_weighted) return has_edge(edge.source, edge.destination, edge.weight);
    return has_edge(edge.source, edge.destination);
}

bool CsrWrapper::has_edge(uint64_t source, uint64_t destination) const {
    auto lower = std::lower_bound(col_ind.begin() + row_ptr[source], col_ind.begin() + row_ptr[source + 1], destination);
    return (lower != col_ind.end()) && (*lower == destination);
}

bool CsrWrapper::has_edge(uint64_t source, uint64_t destination, double weight) const {
    throw driver::error::FunctionNotImplementedError("CsrWrapper::get_neighbors");
}

uint64_t CsrWrapper::degree(uint64_t vertex) const {
    return row_ptr[vertex + 1] - row_ptr[vertex];
}

double CsrWrapper::get_weight(uint64_t source, uint64_t destination) const {
    throw driver::error::FunctionNotImplementedError("CsrWrapper::get_weight");
}

uint64_t CsrWrapper::logical2physical(uint64_t logical) const {
    return logical;
}

uint64_t CsrWrapper::physical2logical(uint64_t physical) const {
    return physical;
}

uint64_t CsrWrapper::vertex_count() const {
    return row_ptr.size() - 1;
}

uint64_t CsrWrapper::edge_count() const {
    return col_ind.size();
}

void CsrWrapper::get_neighbors(uint64_t vertex, std::vector<uint64_t> &neighbors) const {
    auto lower = col_ind.begin() + row_ptr[vertex];
    auto upper = col_ind.begin() + row_ptr[vertex + 1];
    neighbors.insert(neighbors.end(), lower, upper);
}

void CsrWrapper::get_neighbors(uint64_t vertex, std::vector<std::pair<uint64_t, double>> &) const {
    throw driver::error::FunctionNotImplementedError("CsrWrapper::get_neighbors");
}

bool CsrWrapper::insert_vertex(uint64_t vertex) {
    throw driver::error::FunctionNotImplementedError("CsrWrapper::insert_vertex");
}

bool CsrWrapper::insert_edge(uint64_t source, uint64_t destination, double weight) {
    throw driver::error::FunctionNotImplementedError("CsrWrapper::insert_edge");
}

bool CsrWrapper::insert_edge(uint64_t source, uint64_t destination) {
    throw driver::error::FunctionNotImplementedError("CsrWrapper::insert_edge");
}

bool CsrWrapper::remove_vertex(uint64_t vertex) {
    throw driver::error::FunctionNotImplementedError("CsrWrapper::remove_vertex");
}

bool CsrWrapper::remove_edge(uint64_t source, uint64_t destination) {
    throw driver::error::FunctionNotImplementedError("CsrWrapper::remove_edge");
}

bool CsrWrapper::run_batch_vertex_update(std::vector<uint64_t> &vertices, uint64_t start, uint64_t end) {
    row_ptr.resize(end - start + 1);
    // TODO: int -> uint64_t
    return true;
}

bool CsrWrapper::run_batch_edge_update(std::vector<operation> & edges, uint64_t start, uint64_t end, operationType type) {
    std::vector<weightedEdge> edge_list;
    for (uint64_t i = start; i < end; i++) {
        edge_list.push_back(edges[i].e);
        if (!m_is_directed) edge_list.push_back({edges[i].e.destination, edges[i].e.source});
    }
    
    std::sort(edge_list.begin(), edge_list.end(), [] (const weightedEdge &a, const weightedEdge &b) {
        if (a.source == b.source) return a.destination < b.destination;
        else return a.source < b.source;
    });

    if (row_ptr.size() == 0 || edge_list.size() == 0) return true;
    uint64_t prev_source = edge_list[0].source;
    row_ptr[0] = 0;

    col_ind.resize(edge_list.size());
    weight.resize(edge_list.size());

    for (uint64_t i = 0; i < edge_list.size(); i++) {
        col_ind[i] = edge_list[i].destination;
        weight[i] = edge_list[i].weight;

        if (edge_list[i].source != prev_source) {
            for (uint64_t j = prev_source + 1; j <= edge_list[i].source; j++) {
                row_ptr[j] = i;
            }
            prev_source = edge_list[i].source;
        }
    }

    for (uint64_t i = prev_source + 1; i < row_ptr.size(); i++) {
        row_ptr[i] = edge_list.size();
    }
    return true;
}

void CsrWrapper::clear() {
    throw driver::error::FunctionNotImplementedError("CsrWrapper::clear");
}

// Snapshot Related Functions Implementations
std::unique_ptr<CsrWrapper::Snapshot> CsrWrapper::get_unique_snapshot() const {
    auto non_const_this = const_cast<CsrWrapper*>(this);
    return std::make_unique<Snapshot>(*non_const_this);
}

std::shared_ptr<CsrWrapper::Snapshot> CsrWrapper::get_shared_snapshot() const {
    auto non_const_this = const_cast<CsrWrapper*>(this);
    return std::make_shared<Snapshot>(*non_const_this);
}

uint64_t CsrWrapper::Snapshot::size() const {
    return m_graph.vertex_count();
}

uint64_t CsrWrapper::Snapshot::physical2logical(uint64_t physical) const {
    return physical;
}

uint64_t CsrWrapper::Snapshot::logical2physical(uint64_t logical) const {
    return logical;
}

uint64_t CsrWrapper::Snapshot::degree(uint64_t vertex, bool logical) const {
    return m_graph.degree(vertex);
}

bool CsrWrapper::Snapshot::has_vertex(uint64_t vertex) const {
    return m_graph.has_vertex(vertex);
}

bool CsrWrapper::Snapshot::has_edge(weightedEdge edge) const {
    return m_graph.has_edge(edge);
}

bool CsrWrapper::Snapshot::has_edge(uint64_t source, uint64_t destination) const {
    return m_graph.has_edge(source, destination);
}

bool CsrWrapper::Snapshot::has_edge(uint64_t source, uint64_t destination, double weight) const {
    return m_graph.has_edge(source, destination, weight);
}

uint64_t CsrWrapper::Snapshot::intersect(uint64_t vtx_a, uint64_t vtx_b) const {
    return 0;
}

double CsrWrapper::Snapshot::get_weight(uint64_t source, uint64_t destination) const {
    return m_graph.get_weight(source, destination);
}

uint64_t CsrWrapper::Snapshot::vertex_count() const {
    return m_graph.vertex_count();
}

uint64_t CsrWrapper::Snapshot::edge_count() const {
    return m_graph.edge_count();
}

uint64_t CsrWrapper::Snapshot::get_neighbor_addr(uint64_t vertex) const {
    uint64_t value = m_graph.row_ptr[vertex];
    return value;
}

void CsrWrapper::Snapshot::edges(uint64_t vertex, std::vector<uint64_t> &neighbors, bool logical) const {
    neighbors.clear();
    neighbors.reserve(m_graph.degree(vertex));

    auto lower = m_graph.col_ind.begin() + m_graph.row_ptr[vertex];
    auto upper = m_graph.col_ind.begin() + m_graph.row_ptr[vertex + 1];
    neighbors.insert(neighbors.end(), lower, upper);
}

template<typename T>
uint64_t CsrWrapper::Snapshot::edges(uint64_t vertex, T const & callback, bool logical) const {
    auto lower = m_graph.col_ind.begin() + m_graph.row_ptr[vertex];
    auto upper = m_graph.col_ind.begin() + m_graph.row_ptr[vertex + 1];
    // auto weight = m_graph.weight.begin() + m_graph.row_ptr[vertex];

    double weight = 0.0;
    uint64_t sum = 0;
    for (auto it = lower; it != upper; ++it) {
        bool should_continue = callback(*it, weight);
        // sum += *it;
        if (!should_continue) return sum;
    }
    return sum;
}