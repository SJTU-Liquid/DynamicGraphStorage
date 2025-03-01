#include "aspen_wrapper.h"

// Functions for debug purpose
namespace wrapper {
    void wrapper_test() {
        auto wrapper = AspenWrapper();
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
        auto wrapper = AspenWrapper();
        Driver<AspenWrapper, std::shared_ptr<AspenWrapper::Snapshot>> d(wrapper, config);
        d.execute(config.workload_type, config.target_stream_type);
    }
}

#include "driver_main.h"

// Function Implementations
// I

std::shared_ptr<AspenWrapper> AspenWrapper::create_update_interface(std::string graph_type) {
    if (graph_type == "aspen") {
        return std::make_shared<AspenWrapper>();
    }
    else {
        throw std::runtime_error("Graph name not recognized, interfaces::create_update_interface");
    }
}

// Multi-thread
void AspenWrapper::set_max_threads(int max_threads) {}
void AspenWrapper::init_thread(int thread_id) {}
void AspenWrapper::end_thread(int thread_id) {}

// Graph Operations
bool AspenWrapper::is_directed() const {
    return m_is_directed;
}

bool AspenWrapper::is_weighted() const {
    return m_is_weighted;
}

bool AspenWrapper::is_empty() const {
    auto non_const_this = const_cast<AspenWrapper*>(this);
    auto S = non_const_this->m_graph.acquire_version();
    const auto& GA = S.graph;
    bool is_empty = (GA.num_vertices() == 0);
    non_const_this->m_graph.release_version(std::move(S));
    return is_empty;
}

bool AspenWrapper::has_vertex(uint64_t vertex) const {
    throw driver::error::FunctionNotImplementedError("aspenDriver::has_vertex");
}

bool AspenWrapper::has_edge(weightedEdge edge) const {
    auto non_const_this = const_cast<AspenWrapper*>(this);
    auto S = non_const_this->m_graph.acquire_version();
    auto& GA = S.graph;

    auto tmp = GA.find_vertex(edge.source);
    bool flag = tmp.value.contains(edge.source, edge.destination);
    non_const_this->m_graph.release_version(std::move(S));
    return flag;
}

bool AspenWrapper::has_edge(uint64_t source, uint64_t destination) const {
    return has_edge(weightedEdge(source, destination, 1.0));
}

bool AspenWrapper::has_edge(uint64_t source, uint64_t destination, double weight) const {
    throw driver::error::FunctionNotImplementedError("aspenDriver::has_edge");
}

uint64_t AspenWrapper::degree(uint64_t vertex) const {
    auto non_const_this = const_cast<AspenWrapper*>(this);
    auto S = non_const_this->m_graph.acquire_version();
    auto& GA = S.graph;

    uint64_t degree = 0;
    auto tmp = GA.find_vertex(vertex);
    degree = tmp.value.degree();
    non_const_this->m_graph.release_version(std::move(S));
    return degree;
}

double AspenWrapper::get_weight(uint64_t source, uint64_t destination) const {
    throw driver::error::FunctionNotImplementedError("aspenDriver::get_weight");
}

uint64_t AspenWrapper::logical2physical(uint64_t logical) const {
    return logical;
}

uint64_t AspenWrapper::physical2logical(uint64_t physical) const {
    return physical;
}

uint64_t AspenWrapper::vertex_count() const {
    auto non_const_this = const_cast<AspenWrapper*>(this);
    auto S = non_const_this->m_graph.acquire_version();
    const auto& GA = S.graph;
    uint64_t vertex_count = GA.num_vertices();
    non_const_this->m_graph.release_version(std::move(S));
    return vertex_count;
}

uint64_t AspenWrapper::edge_count() const {
    auto non_const_this = const_cast<AspenWrapper*>(this);
    auto S = non_const_this->m_graph.acquire_version();
    const auto& GA = S.graph;
    uint64_t edge_count = GA.num_edges();
    non_const_this->m_graph.release_version(std::move(S));
    return edge_count;
}

void AspenWrapper::get_neighbors(uint64_t vertex, vector<uint64_t> &neighbors) const {
    auto non_const_this = const_cast<AspenWrapper*>(this);
    auto S = non_const_this->m_graph.acquire_version();
    auto& GA = S.graph;

    auto tmp = GA.find_vertex(vertex);
    auto f = [&neighbors](uint64_t src, uint64_t ind) {neighbors.push_back(ind);};
    tmp.value.map_nghs(vertex, f);
    non_const_this->m_graph.release_version(std::move(S));
}

void AspenWrapper::get_neighbors(uint64_t vertex, vector<std::pair<uint64_t, double>> &) const {
    throw driver::error::FunctionNotImplementedError("aspenDriver::get_neighbors");
}

bool AspenWrapper::insert_vertex(uint64_t vertex) {
    return true;
}

bool AspenWrapper::insert_edge(uint64_t source, uint64_t destination, double weight) {
    auto batch = pbbs::new_array_no_init<std::tuple<unsigned int, unsigned int>>(2);
    batch[0] = std::make_pair(source, destination);
    batch[1] = std::make_pair(destination, source);
    m_graph.insert_edges_batch(2, batch, false, false);
    pbbs::free_array(batch);
    return true;
}

bool AspenWrapper::insert_edge(uint64_t source, uint64_t destination) {
    return insert_edge(source, destination, 1.0);
}

bool AspenWrapper::remove_vertex(uint64_t vertex) {
    throw driver::error::FunctionNotImplementedError("aspenDriver::remove_vertex");
}

bool AspenWrapper::remove_edge(uint64_t source, uint64_t destination) {
    auto batch = pbbs::new_array_no_init<std::tuple<unsigned int, unsigned int>>(2);
    batch[0] = std::make_pair(source, destination);
    batch[1] = std::make_pair(destination, source);
    m_graph.delete_edges_batch(2, batch, true, false);
    pbbs::free_array(batch);
    return true;
}

bool AspenWrapper::run_batch_vertex_update(vector<uint64_t> &vertices, uint64_t start, uint64_t end) {
    return true;
}

bool AspenWrapper::run_batch_edge_update(std::vector<operation> & edges, uint64_t start, uint64_t end, operationType type) {
    switch (type) {
        case operationType::INSERT:
            try {
		        uint64_t size = (end - start) * 2;
                if (size == 0) return true;
                auto batch = pbbs::new_array_no_init<std::tuple<unsigned int, unsigned int>>(size);
                for (uint64_t i = start; i < end; i++) {
		            auto edge = edges[i].e;
                    batch[(i - start) * 2] = std::make_pair(edge.source, edge.destination);
                    batch[(i - start) * 2 + 1] = std::make_pair(edge.destination, edge.source);
                }
                m_graph.insert_edges_batch(size, batch, false, false);
                pbbs::free_array(batch);
            } catch (const std::exception& e) {
                return false;
            }
            break;
        
        default:
            throw driver::error::FunctionNotImplementedError("wrong update type in teseo_driver::run_batch_edge_update");
    }
    return true;
}

void AspenWrapper::clear() {
    throw driver::error::FunctionNotImplementedError("aspenDriver::clear");
}

// Snapshot Related Functions Implementations
unique_ptr<AspenWrapper::Snapshot> AspenWrapper::get_unique_snapshot() const {
    auto non_const_this = const_cast<AspenWrapper*>(this);
    return std::make_unique<Snapshot>(&non_const_this->m_graph, vertex_count(), edge_count(), m_is_weighted);
}

shared_ptr<AspenWrapper::Snapshot> AspenWrapper::get_shared_snapshot() const {
    auto non_const_this = const_cast<AspenWrapper*>(this);
    return std::make_shared<Snapshot>(&non_const_this->m_graph, vertex_count(), edge_count(), m_is_weighted);
}

uint64_t AspenWrapper::Snapshot::size() const {
    return this->m_num_vertices;
}

uint64_t AspenWrapper::Snapshot::physical2logical(uint64_t physical) const {
    return physical;
}

uint64_t AspenWrapper::Snapshot::logical2physical(uint64_t logical) const {
    return logical;
}

uint64_t AspenWrapper::Snapshot::degree(uint64_t vertex, bool logical) const {
    auto non_const_this = const_cast<Snapshot*>(this);
    auto& GA = non_const_this->m_version.graph;

    uint64_t degree = 0;
    auto tmp = GA.find_vertex(vertex);
    degree = tmp.value.degree();
    return degree;
}

bool AspenWrapper::Snapshot::has_vertex(uint64_t vertex) const {
    throw driver::error::FunctionNotImplementedError("aspenDriver::has_vertex");
}

bool AspenWrapper::Snapshot::has_edge(weightedEdge edge) const {
    return has_edge(edge.source, edge.destination);
}

uint64_t AspenWrapper::Snapshot::intersect(uint64_t vtx_a, uint64_t vtx_b) const {
    auto non_const_this = const_cast<Snapshot*>(this);
    auto& GA = non_const_this->m_version.graph;

    auto root_a = GA.find_vertex(vtx_a);
    auto root_b = GA.find_vertex(vtx_b);
    return tree_plus::intersect(root_a.value, vtx_a, root_b.value, vtx_b);
}

bool AspenWrapper::Snapshot::has_edge(uint64_t source, uint64_t destination) const {
    auto non_const_this = const_cast<Snapshot*>(this);
    auto& GA = non_const_this->m_version.graph;

    auto tmp = GA.find_vertex(source);
    bool flag = tmp.value.contains(source, destination);
    return flag;
}

bool AspenWrapper::Snapshot::has_edge(uint64_t source, uint64_t destination, double weight) const {
    return has_edge(source, destination);
}

double AspenWrapper::Snapshot::get_weight(uint64_t source, uint64_t destination) const {
    throw driver::error::FunctionNotImplementedError("aspenDriver::get_weight");
}

uint64_t AspenWrapper::Snapshot::vertex_count() const {
    return m_num_vertices;
}

uint64_t AspenWrapper::Snapshot::edge_count() const {
    return m_num_edges;
}

void AspenWrapper::Snapshot::get_neighbor_addr(uint64_t vertex) const {
    auto non_const_this = const_cast<Snapshot*>(this);
    auto& GA = non_const_this->m_version.graph;

    auto tmp = GA.find_vertex(vertex);
    volatile auto deg = tmp.value.degree();
}

void AspenWrapper::Snapshot::edges(uint64_t vertex, vector<uint64_t> &neighbors, bool logical) const {
    auto non_const_this = const_cast<Snapshot*>(this);
    auto& GA = non_const_this->m_version.graph;

    auto tmp = GA.find_vertex(vertex);
    auto f = [&neighbors](uint64_t src, uint64_t ind) {neighbors.push_back(ind);};
    tmp.value.map_nghs(vertex, f);
}

template<typename F>
void AspenWrapper::Snapshot::edges(uint64_t vertex, F&& callback, bool logical) const {
    auto non_const_this = const_cast<Snapshot*>(this);
    auto& GA = non_const_this->m_version.graph;

    auto tmp = GA.find_vertex(vertex);
    auto f = [&callback](uint64_t ind) {callback(ind, 1.0);};
    tmp.value.iter_elms(vertex, f);
}
