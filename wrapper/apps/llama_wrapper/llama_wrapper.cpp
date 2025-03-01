#include "llama_wrapper.h"


// Functions for debug purpose
namespace wrapper {
    void wrapper_test() {
        auto wrapper = LLAMAWrapper();
        std::cout << "This is " << wrapper_repl(wrapper) << "!" << std::endl;
        insert_vertex(wrapper, 0);
        insert_vertex(wrapper, 1);
        insert_vertex(wrapper, 3);
        insert_edge(wrapper, 0, 1);
        insert_edge(wrapper, 0, 3);
        assert(vertex_count(wrapper) == 4);
        assert(is_empty(wrapper) == false);
        assert(has_vertex(wrapper, 0) == true);
        assert(degree(wrapper, 0) == 2);
        assert(has_edge(wrapper, 0, 1) == true);
        std::cout << wrapper_repl(wrapper) << " basic tests passed!" << std::endl;
        auto unique_snapshot = get_unique_snapshot(wrapper);
        assert(snapshot_vertex_count(unique_snapshot) == 4);
        std::cout << wrapper_repl(wrapper) << " snapshot tests passed!" << std::endl;
        std::cout << wrapper_repl(wrapper) << " tests passed!" << std::endl;
    }

    void execute(const DriverConfig & config) {
        auto wrapper = LLAMAWrapper();
        Driver<LLAMAWrapper, std::shared_ptr<LLAMAWrapper::Snapshot>> d(wrapper, config);
        d.execute(config.workload_type, config.target_stream_type);
    }
}

#include "driver_main.h"

// Function Implementations
void LLAMAWrapper::load(const std::string &path, driver::reader::readerType type) {
    auto reader = driver::reader::Reader::open(path, type);
    switch (type) {
        case driver::reader::readerType::edgeList: {
            weightedEdge edge;
            while (reader->read(edge)) {
                try {
                    if (!has_vertex(edge.source)) {
                        insert_vertex(edge.source);
                    }
                    if (!has_vertex(edge.destination)) {
                        insert_vertex(edge.destination);
                    }
                    if (!has_edge(edge.source, edge.destination)) {
                        insert_edge(edge.source, edge.destination);
                    }
                } catch (const std::exception & e) {
                    std::cerr << e.what() << std::endl;
                }
            }
            break;
        }
        case driver::reader::readerType::vertexList: {
            uint64_t vertex;
            while (reader->read(vertex)) {
                try {
                    if (!has_vertex(vertex)) {
                        insert_vertex(vertex);
                    }
                } catch (const std::exception & e) {
                    std::cerr << e.what() << std::endl;
                }
            }
            break;
        }
    }
}

std::shared_ptr<LLAMAWrapper> LLAMAWrapper::create_update_interface(std::string graph_type) {
    if (graph_type == "llama") {
        return std::make_shared<LLAMAWrapper>();
    }
    else {
        throw std::runtime_error("Graph name not recognized, interfaces::create_update_interface");
    }
}

void LLAMAWrapper::set_max_threads(int max_threads) {}
void LLAMAWrapper::init_thread(int thread_id) {}
void LLAMAWrapper::end_thread(int thread_id) {}

bool LLAMAWrapper::is_directed() const {
    return m_is_directed;
}

bool LLAMAWrapper::is_weighted() const {
    return m_is_weighted;
}

bool LLAMAWrapper::is_empty() const {
    return (this->vertex_count() == 0);
}

bool LLAMAWrapper::has_vertex(uint64_t vertex) const {
    ll_writable_graph& graph = *m_database->graph();
    graph.tx_begin();
    bool exists = graph.node_exists(vertex);
    graph.tx_commit();
    return exists;
}

bool LLAMAWrapper::has_edge(weightedEdge edge) const {
    return has_edge(edge.source, edge.destination);
}

bool LLAMAWrapper::has_edge(uint64_t source, uint64_t destination) const {
    auto *non_const_this = const_cast<LLAMAWrapper *>(this);
    ll_mlcsr_ro_graph* snapshot = get_llama_snapshot(m_database);

    bool result = snapshot->find(source, destination) != LL_NIL_EDGE;
    free(snapshot);
    return result;
}

bool LLAMAWrapper::has_edge(uint64_t source, uint64_t destination, double weight) const {
    auto *non_const_this = const_cast<LLAMAWrapper *>(this);
    ll_mlcsr_ro_graph* snapshot = get_llama_snapshot(non_const_this->m_database);
    ll_edge_iterator iter{};

    snapshot->out_iter_begin(iter, source);
    for (edge_t s_idx = snapshot->out_iter_next(iter); s_idx != LL_NIL_EDGE; s_idx = snapshot->out_iter_next(iter)) {
        node_t v = iter.last_node;
        if (v == destination) {
            uint64_t w = 0;
            char const * const g_llama_property_weights = "weights";
            w = reinterpret_cast<ll_mlcsr_edge_property<uint64_t>*>(snapshot->get_edge_property_64(g_llama_property_weights))->get(s_idx);
            free(snapshot);
            return (double) w == weight;
        }
    }
    free(snapshot);
    return false;
}

uint64_t LLAMAWrapper::degree(uint64_t vertex) const {
    auto *non_const_this = const_cast<LLAMAWrapper *>(this);
    ll_mlcsr_ro_graph* snapshot = get_llama_snapshot(non_const_this->m_database);
    uint64_t degree = snapshot->out_degree(vertex);
    free(snapshot);
    return degree;
}

double LLAMAWrapper::get_weight(uint64_t source, uint64_t destination) const {
    auto *non_const_this = const_cast<LLAMAWrapper *>(this);
    ll_mlcsr_ro_graph* snapshot = get_llama_snapshot(non_const_this->m_database);
    ll_edge_iterator iter{};

    ll_mlcsr_ro_graph &ss = *snapshot;

    snapshot->out_iter_begin(iter, source);
    for (edge_t s_idx = snapshot->out_iter_next(iter); s_idx != LL_NIL_EDGE; s_idx = snapshot->out_iter_next(iter)) {
        node_t v = LL_ITER_OUT_NEXT_NODE(ss, iter, s_idx);
        if (v == destination) {
            uint64_t w = 0;
            char const * const g_llama_property_weights = "weights";
            w = reinterpret_cast<ll_mlcsr_edge_property<uint64_t>*>(snapshot->get_edge_property_64(g_llama_property_weights))->get(s_idx);
            free(snapshot);
            return (double) w;
        }
    }
    free(snapshot);
    throw driver::error::GraphLogicalError("Edge does not exist : llamaDriver::get_weight");
}

uint64_t LLAMAWrapper::logical2physical(uint64_t logical_id) const {
    return logical_id;
}

uint64_t LLAMAWrapper::physical2logical(uint64_t physical_id) const {
    return physical_id;
}

uint64_t LLAMAWrapper::vertex_count() const {
    auto *non_const_this = const_cast<LLAMAWrapper *>(this);
    ll_mlcsr_ro_graph* snapshot = get_llama_snapshot(non_const_this->m_database);
    auto vertex_cnt = snapshot->max_nodes();
    free(snapshot);
    return vertex_cnt;
}

uint64_t LLAMAWrapper::edge_count() const {
    auto *non_const_this = const_cast<LLAMAWrapper *>(this);
    ll_mlcsr_ro_graph* snapshot = get_llama_snapshot(non_const_this->m_database);
    auto result = snapshot->max_edges(snapshot->num_levels() - 1);
    free(snapshot);
    return result;
}

void LLAMAWrapper::get_neighbors(uint64_t vertex, std::vector<uint64_t> &neighbors) const {
    auto *non_const_this = const_cast<LLAMAWrapper *>(this);
    ll_mlcsr_ro_graph* snapshot = get_llama_snapshot(non_const_this->m_database);
    ll_edge_iterator iter{};

    snapshot->out_iter_begin(iter, vertex);
    for (edge_t s_idx = snapshot->out_iter_next(iter); s_idx != LL_NIL_EDGE; s_idx = snapshot->out_iter_next(iter)) {
        node_t v = iter.last_node;
        neighbors.push_back(v);
    }
    free(snapshot);
}

void LLAMAWrapper::get_neighbors(uint64_t vertex, std::vector<std::pair<uint64_t, double>> &) const {
    throw driver::error::FunctionNotImplementedError("get_neighbors");
}

bool LLAMAWrapper::insert_vertex(uint64_t vertex) {
    ll_writable_graph& graph = *m_database->graph();
    graph.tx_begin();
    graph.add_node();
    graph.checkpoint();
    graph.tx_commit();
    return true;
}

bool LLAMAWrapper::insert_edge(uint64_t source, uint64_t destination, double weight) {
    ll_writable_graph& graph = *m_database->graph();

    char const * const g_llama_property_weights = "w";
    graph.tx_begin();

    auto edge_id = graph.add_edge(source, destination);
    graph.get_edge_property_64(g_llama_property_weights)->set(edge_id, *reinterpret_cast<uint64_t*>(&(weight)));

    if (!m_is_directed) {
        edge_id = graph.add_edge(destination, source);
        graph.get_edge_property_64(g_llama_property_weights)->set(edge_id, *reinterpret_cast<uint64_t*>(&(weight)));
    }

    graph.checkpoint();
    graph.tx_commit();
    return true;
}

bool LLAMAWrapper::insert_edge(uint64_t source, uint64_t destination) {
    ll_writable_graph& graph = *m_database->graph();
    graph.tx_begin();
    graph.add_edge(source, destination);
    if (!m_is_directed) graph.add_edge(destination, source);
    graph.checkpoint();
    graph.tx_commit();
    
    
    return true;
}

bool LLAMAWrapper::remove_vertex(uint64_t vertex) {
    ll_writable_graph& graph = *m_database->graph();
    graph.tx_begin();
    graph.delete_node(vertex);
    graph.checkpoint();
    graph.tx_commit();
    return true;
}

bool LLAMAWrapper::remove_edge(uint64_t source, uint64_t destination) {
    ll_writable_graph& graph = *m_database->graph();
    graph.tx_begin();
    graph.delete_edge(source, graph.find(source, destination));
    graph.tx_commit();
    graph.checkpoint();

    return true;
}

bool LLAMAWrapper::run_batch_vertex_update(std::vector<uint64_t> &vertices, uint64_t start, uint64_t end) {
    ll_writable_graph& graph = *m_database->graph();
    graph.tx_begin();
    for (int i = start; i < end; i++) {
        graph.add_node();
    }
    graph.checkpoint();
    graph.tx_commit();
    return true;
}

bool LLAMAWrapper::run_batch_edge_update(std::vector<operation> & edges, uint64_t start, uint64_t end, operationType type) {
    switch (type) {
        case operationType::INSERT:
            try {
                char const * const g_llama_property_weights = "weights";
                ll_writable_graph& graph = *m_database->graph();
                graph.tx_begin();

                for (uint64_t i = start; i < end; i++) {
                    double weight = edges[i].e.weight;
                    edge_t edge_id = graph.add_edge(edges[i].e.source, edges[i].e.destination);
                    if (m_is_weighted) {
                        graph.get_edge_property_64(g_llama_property_weights)->set(edge_id, *reinterpret_cast<uint64_t*>(&weight));
                    }
                }
                if (!m_is_directed) {
                    for (uint64_t i = start; i < end; i++) {
                        double weight = edges[i].e.weight;
                        edge_t edge_id = graph.add_edge(edges[i].e.destination, edges[i].e.source);
                        if (m_is_weighted) {
                            graph.get_edge_property_64(g_llama_property_weights)->set(edge_id, *reinterpret_cast<uint64_t*>(&weight));
                        }
                    }
                }
                graph.checkpoint();
                graph.tx_commit();
            } catch (...) {
                return false;
            }
            break;
        case operationType::DELETE:
            try {
                ll_writable_graph& graph = *m_database->graph();
                graph.tx_begin();

                for (uint64_t i = start; i < end; i++) {
                    graph.delete_edge(edges[i].e.source, graph.find(edges[i].e.source, edges[i].e.destination));
                }
                if (!m_is_directed) {
                    for (uint64_t i = start; i < end; i++) {
                        graph.delete_edge(edges[i].e.destination, graph.find(edges[i].e.destination, edges[i].e.source));
                    }
                }
                graph.checkpoint();
                graph.tx_commit();
            } catch (...) {
                return false;
            }
            break;
    }
    return true;
}

void LLAMAWrapper::clear() {}

std::unique_ptr<LLAMAWrapper::Snapshot> LLAMAWrapper::get_unique_snapshot() const {
    auto *non_const_this = const_cast<LLAMAWrapper *>(this);
    return std::make_unique<Snapshot>(get_llama_snapshot(non_const_this->m_database), m_is_directed, m_is_weighted);
}

std::shared_ptr<LLAMAWrapper::Snapshot> LLAMAWrapper::get_shared_snapshot() const {
    auto *non_const_this = const_cast<LLAMAWrapper *>(this);
    return std::make_shared<Snapshot>(get_llama_snapshot(non_const_this->m_database), m_is_directed, m_is_weighted);
}

// Snapshot Related Function Implementations
uint64_t LLAMAWrapper::Snapshot::size() const {
    return m_snapshot->max_nodes();
}

uint64_t LLAMAWrapper::Snapshot::physical2logical(uint64_t physical) const {
    return physical;
}

uint64_t LLAMAWrapper::Snapshot::logical2physical(uint64_t logical) const {
    return logical;
}

uint64_t LLAMAWrapper::Snapshot::degree(uint64_t vertex, bool logical) const {
    return m_snapshot->out_degree(vertex);
}

bool LLAMAWrapper::Snapshot::has_vertex(uint64_t vertex) const {
    return m_snapshot->node_exists(vertex);
}

bool LLAMAWrapper::Snapshot::has_edge(weightedEdge edge) const {
    if (m_is_weighted) {
        return has_edge(edge.source, edge.destination, edge.weight);
    }
    else {
        return has_edge(edge.source, edge.destination);
    }
}

bool LLAMAWrapper::Snapshot::has_edge(uint64_t source, uint64_t destination) const {
     auto non_const_this = const_cast<Snapshot*>(this);
    ll_edge_iterator iter{};

    m_snapshot->out_iter_begin(iter, source);

    for (edge_t s_idx = m_snapshot->out_iter_next(iter); s_idx != LL_NIL_EDGE; s_idx = m_snapshot->out_iter_next(iter)) {
        node_t v = iter.last_node;
        if (v == destination) {
            return true;
        }
    }
    return false;
}

bool LLAMAWrapper::Snapshot::has_edge(uint64_t source, uint64_t destination, double weight) const {
    auto non_const_this = const_cast<Snapshot*>(this);
    ll_edge_iterator iter{};

    m_snapshot->out_iter_begin(iter, source);

    for (edge_t s_idx = m_snapshot->out_iter_next(iter); s_idx != LL_NIL_EDGE; s_idx = m_snapshot->out_iter_next(iter)) {
        node_t v = iter.last_node;
        if (v == destination) {
            uint64_t w = 0;
            char const * const g_llama_property_weights = "weights";
            w = reinterpret_cast<ll_mlcsr_edge_property<uint64_t>*>(non_const_this->m_snapshot->get_edge_property_64(g_llama_property_weights))->get(s_idx);
            return (double) w == weight;
        }
    }
    return false;
}

double LLAMAWrapper::Snapshot::get_weight(uint64_t source, uint64_t destination) const {
    auto non_const_this = const_cast<Snapshot*>(this);
    ll_edge_iterator iter{};

    m_snapshot->out_iter_begin(iter, source);

    for (edge_t s_idx = m_snapshot->out_iter_next(iter); s_idx != LL_NIL_EDGE; s_idx = m_snapshot->out_iter_next(iter)) {
        node_t v = iter.last_node;
        if (v == destination) {
            uint64_t w = 0;
            char const * const g_llama_property_weights = "weights";
            w = reinterpret_cast<ll_mlcsr_edge_property<uint64_t>*>(non_const_this->m_snapshot->get_edge_property_64(g_llama_property_weights))->get(s_idx);
            return (double) w;
        }
    }
    throw driver::error::GraphLogicalError("Edge does not exist : llamaDriver::get_weight");
}

uint64_t LLAMAWrapper::Snapshot::vertex_count() const {
    return m_snapshot->max_nodes();
}

uint64_t LLAMAWrapper::Snapshot::edge_count() const {
    return m_snapshot->max_edges(m_snapshot->num_levels() - 1);
}

void LLAMAWrapper::Snapshot::get_neighbor_addr(uint64_t index) const {
    auto non_const_this = const_cast<Snapshot*>(this);

    ll_edge_iterator iter{};
    non_const_this->m_snapshot->out_iter_begin(iter, index);
}

void LLAMAWrapper::Snapshot::edges(uint64_t vertex, std::vector<uint64_t> &neighbors, bool logical) const {
    auto non_const_this = const_cast<Snapshot*>(this);

    ll_edge_iterator iter{};
    non_const_this->m_snapshot->out_iter_begin(iter, vertex);

    for (edge_t s_idx = non_const_this->m_snapshot->out_iter_next(iter); s_idx != LL_NIL_EDGE; s_idx = non_const_this->m_snapshot->out_iter_next(iter)) {
        node_t v = iter.last_node;
        neighbors.push_back(v);
    }
}

void LLAMAWrapper::Snapshot::edges(uint64_t vertex, std::function<void(uint64_t, double)> callback, bool logical) const {
    auto non_const_this = const_cast<Snapshot*>(this);

    ll_edge_iterator iter{};
    non_const_this->m_snapshot->out_iter_begin(iter, vertex);

    for (edge_t s_idx = non_const_this->m_snapshot->out_iter_next(iter); s_idx != LL_NIL_EDGE; s_idx = non_const_this->m_snapshot->out_iter_next(iter)) {
        node_t v = iter.last_node;
        uint64_t w = 0;
        char const * const g_llama_property_weights = "weights";
        
        w = reinterpret_cast<ll_mlcsr_edge_property<uint64_t>*>(non_const_this->m_snapshot->get_edge_property_64(g_llama_property_weights))->get(s_idx);
        callback(v, double(w));
    }
}

