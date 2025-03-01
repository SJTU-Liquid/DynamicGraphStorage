#include "livegraph_wrapper.h"

// Functions for debug purpose
namespace wrapper {
    void wrapper_test() {
        auto wrapper = LiveGraphWrapper();
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
        auto cloned_snapshot = snapshot_clone(unique_snapshot);
        assert(snapshot_vertex_count(unique_snapshot) == 3);
        assert(snapshot_vertex_count(cloned_snapshot) == 3);
        std::cout << wrapper_repl(wrapper) << " snapshot tests passed!" << std::endl;
        std::cout << wrapper_repl(wrapper) << " tests passed!" << std::endl;
    }

    void execute(const DriverConfig & config) {
        auto wrapper = LiveGraphWrapper();
        Driver<LiveGraphWrapper, std::shared_ptr<LiveGraphWrapper::Snapshot>> d(wrapper, config);
        d.execute(config.workload_type, config.target_stream_type);
    }
}

#include "driver_main.h"

// Function implementations
std::shared_ptr<LiveGraphWrapper> LiveGraphWrapper::create_update_interface(std::string graph_type) {
    if (graph_type == "livegraph") {
        return std::make_shared<LiveGraphWrapper>();
    }
    else {
        throw std::runtime_error("Graph name not recognized, interfaces::create_update_interface");
    }
}

void LiveGraphWrapper::set_max_threads(int) {}
void LiveGraphWrapper::init_thread(int) {}
void LiveGraphWrapper::end_thread(int) {}

bool LiveGraphWrapper::is_directed() const {
    return m_is_directed;
}

bool LiveGraphWrapper::is_weighted() const {
    return m_is_weighted;
}

bool LiveGraphWrapper::is_empty() const {
    return vertex_count() == 0;
}

bool LiveGraphWrapper::has_vertex(uint64_t vertex) const {
    vertex_dictionary_t::const_accessor accessor;
    return VertexDictionary->find(accessor, vertex);
}

bool LiveGraphWrapper::has_edge(weightedEdge edge) const {
    if (m_is_weighted) {
        return has_edge(edge.source, edge.destination, edge.weight);
    } else {
        return has_edge(edge.source, edge.destination);
    }
}

bool LiveGraphWrapper::has_edge(uint64_t source, uint64_t destination) const {
    vertex_dictionary_t::const_accessor slock1, slock2;
    if (!reinterpret_cast<vertex_dictionary_t*>(m_pHashMap)->find(slock1, source)) { return false; }
    if (!reinterpret_cast<vertex_dictionary_t*>(m_pHashMap)->find(slock2, destination)) { return false; }
    lg::vertex_t internal_source_id = slock1->second;
    lg::vertex_t internal_destination_id = slock2->second;

    auto tx = reinterpret_cast<lg::Graph*>(m_pImpl)->begin_read_only_transaction();
    std::string_view lg_weight = tx.get_edge(internal_source_id, /* label */ 0, internal_destination_id);
    tx.abort();

    return (!lg_weight.empty());
}

bool LiveGraphWrapper::has_edge(uint64_t source, uint64_t destination, double weight) const {
    vertex_dictionary_t::const_accessor slock1, slock2;
    if (!reinterpret_cast<vertex_dictionary_t*>(m_pHashMap)->find(slock1, source)) { return false; }
    if (!reinterpret_cast<vertex_dictionary_t*>(m_pHashMap)->find(slock2, destination)) { return false; }
    lg::vertex_t internal_source_id = slock1->second;
    lg::vertex_t internal_destination_id = slock2->second;

    auto tx = reinterpret_cast<lg::Graph*>(m_pImpl)->begin_read_only_transaction();
    std::string_view lg_weight = tx.get_edge(internal_source_id, /* label */ 0, internal_destination_id);

    tx.abort();

    double internal_weight = *(reinterpret_cast<const double*>(lg_weight.data()));
    return (!lg_weight.empty() && internal_weight == weight);
}

uint64_t LiveGraphWrapper::degree(uint64_t vertex) const {
    vertex_dictionary_t::const_accessor slock;
    if(!reinterpret_cast<vertex_dictionary_t*>(m_pHashMap)->find(slock, vertex)) throw driver::error::GraphLogicalError("Vertex does not exist, livegraphDriver::degree");
    lg::vertex_t internal_vertex_id = slock->second;

    auto tx = reinterpret_cast<lg::Graph*>(m_pImpl)->begin_read_only_transaction();
    uint64_t degree = 0;

    auto iterator = tx.get_edges(internal_vertex_id, /* label */ 0);
    while (iterator.valid()) {
        degree++;
        iterator.next();
    }

    tx.abort();

    return degree;
}

double LiveGraphWrapper::get_weight(uint64_t source, uint64_t destination) const {
    vertex_dictionary_t::const_accessor slock1, slock2;
    if (!reinterpret_cast<vertex_dictionary_t*>(m_pHashMap)->find(slock1, source)){ throw driver::error::GraphLogicalError("Vertex does not exist, livegraphDriver::get_weight"); }
    if (!reinterpret_cast<vertex_dictionary_t*>(m_pHashMap)->find(slock2, destination)){ throw driver::error::GraphLogicalError("Vertex does not exist, livegraphDriver::get_weight"); }
    lg::vertex_t internal_source_id = slock1->second;
    lg::vertex_t internal_destination_id = slock2->second;

    auto tx = reinterpret_cast<lg::Graph*>(m_pImpl)->begin_read_only_transaction();
    std::string_view lg_weight = tx.get_edge(internal_source_id, /* label */ 0, internal_destination_id);
    tx.abort();

    if (lg_weight.empty()) throw driver::error::GraphLogicalError("Edge does not exist, livegraphDriver::get_weight");
    return *(reinterpret_cast<const double*>(lg_weight.data()));
}

uint64_t LiveGraphWrapper::logical2physical(uint64_t vertex) const {
    vertex_dictionary_t::const_accessor slock;
    if(!reinterpret_cast<vertex_dictionary_t*>(m_pHashMap)->find(slock, vertex)) throw driver::error::GraphLogicalError("Vertex does not exist, livegraphDriver::logical2physical");
    return slock->second;
}

uint64_t LiveGraphWrapper::physical2logical(uint64_t physical) const {
    auto transaction = reinterpret_cast<lg::Graph*>(m_pImpl)->begin_read_only_transaction();
    std::string_view lg_vertex = transaction.get_vertex(physical);
    transaction.abort();
    if (lg_vertex.empty()) throw driver::error::GraphLogicalError("Vertex does not exist, livegraphDriver::physical2logical");
    return *(reinterpret_cast<const uint64_t*>(lg_vertex.data()));
}

uint64_t LiveGraphWrapper::vertex_count() const {
    return m_vertex_count;
}

uint64_t LiveGraphWrapper::edge_count() const {
    return m_edge_count;
}

void LiveGraphWrapper::get_neighbors(uint64_t vertex, std::vector<uint64_t> &neighbors) const {
    auto tx = reinterpret_cast<lg::Graph*>(m_pImpl)->begin_read_only_transaction();
    lg::vertex_t internal_vertex_id = logical2physical(vertex);
    auto iterator = tx.get_edges(internal_vertex_id, /* label */ 0);
    while (iterator.valid()) {
        uint64_t neighbor = physical2logical(iterator.dst_id());
        neighbors.push_back(neighbor);
        iterator.next();
    }

    tx.abort();
}

void LiveGraphWrapper::get_neighbors(uint64_t vertex, std::vector<std::pair<uint64_t, double>> &) const {
    throw driver::error::FunctionNotImplementedError("livegraphDriver::get_neighbors");
}

bool LiveGraphWrapper::insert_vertex(uint64_t vertex) {
    vertex_dictionary_t::accessor accessor;
    bool inserted = VertexDictionary->insert(accessor, vertex);
    if (inserted) {
        lg::vertex_t internal_id = 0;
        bool done = false;
        do {
            try {
                auto tx = reinterpret_cast<lg::Graph*>(m_pImpl)->begin_transaction();
                internal_id = tx.new_vertex();
                std::string_view data {(char*) &vertex, sizeof(vertex)};
                tx.put_vertex(internal_id, data);
                tx.commit();
                done = true;
            } catch(lg::Transaction::RollbackExcept& e) {
                // std::cerr << "livegraphDriver::insert_vertex: Transaction failed, retrying" << std::endl;
            }
        } while(!done);

        accessor->second = internal_id;
        m_vertex_count++;
    }

    return inserted;
}

bool LiveGraphWrapper::insert_edge(uint64_t source, uint64_t destination, double weight) {
    vertex_dictionary_t::const_accessor slock1, slock2;
    if (!reinterpret_cast<vertex_dictionary_t*>(m_pHashMap)->find(slock1, source)) { throw driver::error::GraphLogicalError("Vertex does not exist, livegraphDriver::get_weight"); }
    if (!reinterpret_cast<vertex_dictionary_t*>(m_pHashMap)->find(slock2, destination)) { throw driver::error::GraphLogicalError("Vertex does not exist, livegraphDriver::get_weight"); }
    lg::vertex_t internal_source_id = slock1->second;
    lg::vertex_t internal_destination_id = slock2->second;

    bool done = false;
    do {
        try {
            auto tx = reinterpret_cast<lg::Graph*>(m_pImpl)->begin_transaction();
            auto lg_weight = tx.get_edge(internal_source_id, 0, internal_destination_id);
            if(lg_weight.size() > 0){ // the edge already exists
                tx.abort();
                return false;
            }
            std::string_view weight {(char*) &weight, sizeof(weight)};
            tx.put_edge(internal_source_id, /* label */ 0, internal_destination_id, weight);
            if(!m_is_directed) {
                tx.put_edge(internal_destination_id, /* label */ 0, internal_source_id, weight);
            }
            tx.commit();
            m_edge_count++;
            done = true;
        } catch (lg::Transaction::RollbackExcept& exc){
            // std::cerr << "livegraphDriver::insert_edge: Transaction failed, retrying" << std::endl;
        }
    } while(!done);

    return true;
}

bool LiveGraphWrapper::insert_edge(uint64_t source, uint64_t destination) {
    return insert_edge(source, destination, 1.0);
}

bool LiveGraphWrapper::remove_vertex(uint64_t vertex) {
    vertex_dictionary_t::accessor accessor; // xlock
    bool found = VertexDictionary->find(accessor, vertex);
    if (found) {
        lg::vertex_t internal_id = accessor->second;
        bool done = false;
        do {
            try {
                auto tx = reinterpret_cast<lg::Graph*>(m_pImpl)->begin_transaction();
                tx.del_vertex(internal_id);
                tx.commit();
                done = true;
            } catch (lg::Transaction::RollbackExcept& e){
                // retry ...
            }
        } while(!done);

        VertexDictionary->erase(accessor);
    }
    m_vertex_count--;
    return found;
}

bool LiveGraphWrapper::remove_edge(uint64_t source, uint64_t destination) {
    vertex_dictionary_t::const_accessor slock1, slock2;
    if(!VertexDictionary->find(slock1, source)) {return false;}
    if(!VertexDictionary->find(slock2, destination)) {return false;}
    lg::vertex_t internal_source_id = slock1->second;
    lg::vertex_t internal_destination_id = slock2->second;

    while (true) {
        try {
            auto tx = reinterpret_cast<lg::Graph*>(m_pImpl)->begin_transaction();
            bool removed = tx.del_edge(internal_source_id, /* label */ 0, internal_destination_id);
            if(removed && !m_is_directed){ // undirected graph
                tx.del_edge(internal_destination_id, /* label */ 0, internal_source_id);
            }
            tx.commit();
            if (removed) { m_edge_count--; }
            return removed;
        } catch (lg::Transaction::RollbackExcept& e) {
            // retry ...
        }
    }
}

bool LiveGraphWrapper::run_batch_vertex_update(std::vector<uint64_t> &vertices, uint64_t start, uint64_t end) {
    for (int i = start; i < end; i++) {
        insert_vertex(vertices[i]);
    }
    return true;
}

bool LiveGraphWrapper::run_batch_edge_update(std::vector<operation> & edges, uint64_t start, uint64_t end,
                                             operationType type) {
    switch (type) {
        case operationType::INSERT:
            try {
                auto tx = reinterpret_cast<lg::Graph*>(m_pImpl)->begin_transaction();

                for (uint64_t i = start; i < end; i++) {
                    uint64_t source = edges[i].e.source;
                    uint64_t destination = edges[i].e.destination;

                    vertex_dictionary_t::const_accessor slock1, slock2;
                    if (!reinterpret_cast<vertex_dictionary_t*>(m_pHashMap)->find(slock1, source)) { throw driver::error::GraphLogicalError("Vertex does not exist, livegraphDriver::get_weight"); }
                    if (!reinterpret_cast<vertex_dictionary_t*>(m_pHashMap)->find(slock2, destination)) { throw driver::error::GraphLogicalError("Vertex does not exist, livegraphDriver::get_weight"); }
                    lg::vertex_t internal_source_id = slock1->second;
                    lg::vertex_t internal_destination_id = slock2->second;

                    bool done = false;
                    do {
                        try {
                            auto lg_weight = tx.get_edge(internal_source_id, 0, internal_destination_id);
                            if(lg_weight.size() > 0){ // the edge already exists
                                tx.abort();
                                return false;
                            }

                            std::string_view weight {(char*) &weight, sizeof(weight)};
                            tx.put_edge(internal_source_id, /* label */ 0, internal_destination_id, weight);
                            if(!m_is_directed) { // undirected graph
                                tx.put_edge(internal_destination_id, /* label */ 0, internal_source_id, weight);
                            }
                            m_edge_count++;
                            done = true;
                        } catch (...) {

                        }
                    } while(!done);
                }
                tx.commit();

            } catch (...) {
                return false;
            }
            break;

        case operationType::DELETE:
            try {
                auto tx = reinterpret_cast<lg::Graph*>(m_pImpl)->begin_batch_loader();

                for (uint64_t i = start; i < end; i++) {
                    uint64_t source = edges[i].e.source;
                    uint64_t destination = edges[i].e.destination;

                    vertex_dictionary_t::const_accessor slock1, slock2;
                    if (!reinterpret_cast<vertex_dictionary_t*>(m_pHashMap)->find(slock1, source)) { throw driver::error::GraphLogicalError("Vertex does not exist, livegraphDriver::get_weight"); }
                    if (!reinterpret_cast<vertex_dictionary_t*>(m_pHashMap)->find(slock2, destination)) { throw driver::error::GraphLogicalError("Vertex does not exist, livegraphDriver::get_weight"); }
                    lg::vertex_t internal_source_id = slock1->second;
                    lg::vertex_t internal_destination_id = slock2->second;

                    bool done = false;
                    do {
                        try {
                            auto tx = reinterpret_cast<lg::Graph*>(m_pImpl)->begin_transaction();
                            bool removed = tx.del_edge(internal_source_id, /* label */ 0, internal_destination_id);
                            if(removed && !m_is_directed){ // undirected graph
                                tx.del_edge(internal_destination_id, /* label */ 0, internal_source_id);
                            }

                            if (removed) { m_edge_count--;}
                            done = removed;
                        } catch (lg::Transaction::RollbackExcept& e) {
                            // retry ...
                        }
                    } while (!done);
                }

                tx.commit();

            } catch (...) {
                return false;
            }
            break;
    }
    return true;
}

void LiveGraphWrapper::clear() {
    throw driver::error::FunctionNotImplementedError("livegraphDriver::clear");
}

// Snapshot Related Functions Implementation
uint64_t LiveGraphWrapper::Snapshot::size() const {
    return this->m_num_vertices;
}

uint64_t LiveGraphWrapper::Snapshot::logical2physical(uint64_t logical) const {
    vertex_dictionary_t::const_accessor slock;
    if(!reinterpret_cast<vertex_dictionary_t*>(m_pHashMap)->find(slock, logical)) throw driver::error::GraphLogicalError("Vertex does not exist, livegraphDriver::logical2physical");
    return slock->second;
}

uint64_t LiveGraphWrapper::Snapshot::physical2logical(uint64_t physical) const {
    auto non_const_this = const_cast<Snapshot*>(this);
    std::string_view lg_vertex = non_const_this->m_transaction.get_vertex(physical);
    if (lg_vertex.empty()) throw driver::error::GraphLogicalError("Vertex does not exist, livegraphDriver::physical2logical");
    return *(reinterpret_cast<const uint64_t*>(lg_vertex.data()));
}

uint64_t LiveGraphWrapper::Snapshot::degree(uint64_t vertex, bool logical) const {
    auto non_const_this = const_cast<Snapshot*>(this);
    if (logical) {
        vertex = logical2physical(vertex);
    }

    uint64_t degree = 0;
    auto iterator = non_const_this->m_transaction.get_edges(vertex, /* label */ 0);
    while (iterator.valid()) {
        degree++;
        iterator.next();
    }
    return degree;
}

bool LiveGraphWrapper::Snapshot::has_vertex(uint64_t vertex) const {
    vertex_dictionary_t::const_accessor accessor;
    return VertexDictionary->find(accessor, vertex);
}

bool LiveGraphWrapper::Snapshot::has_edge(weightedEdge edge) const {
    if (m_is_weighted) {
        return has_edge(edge.source, edge.destination, edge.weight);
    }
    else {
        return has_edge(edge.source, edge.destination);
    }
}

bool LiveGraphWrapper::Snapshot::has_edge(uint64_t source, uint64_t destination) const {
    auto non_const_this = const_cast<Snapshot*>(this);
    vertex_dictionary_t::const_accessor slock1, slock2;
    if (!reinterpret_cast<vertex_dictionary_t*>(m_pHashMap)->find(slock1, source)) { return false; }
    if (!reinterpret_cast<vertex_dictionary_t*>(m_pHashMap)->find(slock2, destination)) { return false; }
    lg::vertex_t internal_source_id = slock1->second;
    lg::vertex_t internal_destination_id = slock2->second;

    std::string_view lg_weight = non_const_this->m_transaction.get_edge(internal_source_id, /* label */ 0, internal_destination_id);

    return !lg_weight.empty();
}

bool LiveGraphWrapper::Snapshot::has_edge(uint64_t source, uint64_t destination, double weight) const {
    if (!m_is_weighted) {
        throw driver::error::GraphLogicalError("Graph is not m_is_weighted : livegraphDriver::has_edge");
    }

    auto non_const_this = const_cast<Snapshot*>(this);

    vertex_dictionary_t::const_accessor slock1, slock2;
    if (!reinterpret_cast<vertex_dictionary_t*>(m_pHashMap)->find(slock1, source)) { return false; }
    if (!reinterpret_cast<vertex_dictionary_t*>(m_pHashMap)->find(slock2, destination)) { return false; }
    lg::vertex_t internal_source_id = slock1->second;
    lg::vertex_t internal_destination_id = slock2->second;

    std::string_view lg_weight = non_const_this->m_transaction.get_edge(internal_source_id, /* label */ 0, internal_destination_id);

    double internal_weight = *(reinterpret_cast<const double*>(lg_weight.data()));
    return (!lg_weight.empty() && internal_weight == weight);
}

double LiveGraphWrapper::Snapshot::get_weight(uint64_t source, uint64_t destination) const {
    if (!m_is_weighted) {
        throw driver::error::GraphLogicalError("Graph is not m_is_weighted : livegraphDriver::get_weight");
    }

    auto non_const_this = const_cast<Snapshot*>(this);

    vertex_dictionary_t::const_accessor slock1, slock2;
    if (!reinterpret_cast<vertex_dictionary_t*>(m_pHashMap)->find(slock1, source)){ throw driver::error::GraphLogicalError("Vertex does not exist, livegraphDriver::get_weight"); }
    if (!reinterpret_cast<vertex_dictionary_t*>(m_pHashMap)->find(slock2, destination)){ throw driver::error::GraphLogicalError("Vertex does not exist, livegraphDriver::get_weight"); }
    lg::vertex_t internal_source_id = slock1->second;
    lg::vertex_t internal_destination_id = slock2->second;

    std::string_view lg_weight = non_const_this->m_transaction.get_edge(internal_source_id, /* label */ 0, internal_destination_id);

    if (lg_weight.empty()) throw driver::error::GraphLogicalError("Edge does not exist, livegraphDriver::get_weight");
    return *(reinterpret_cast<const double*>(lg_weight.data()));
}

uint64_t LiveGraphWrapper::Snapshot::vertex_count() const {
    return m_num_vertices;
}

uint64_t LiveGraphWrapper::Snapshot::edge_count() const {
    return m_num_edges;
}

void LiveGraphWrapper::Snapshot::get_neighbor_addr(uint64_t index) const {
    auto non_const_this = const_cast<Snapshot*>(this);

    auto iterator = non_const_this->m_transaction.get_edges(index, /* label */ 0);
}

void LiveGraphWrapper::Snapshot::edges(uint64_t index, std::vector<uint64_t> &neighbors, bool logical) const {
    auto non_const_this = const_cast<Snapshot*>(this);

    if (logical) {
        index = logical2physical(index);
    }

    auto iterator = non_const_this->m_transaction.get_edges(index, /* label */ 0);
    while (iterator.valid()) {
        uint64_t neighbor = iterator.dst_id();
        if (logical) {
            neighbor = physical2logical(neighbor);
        }
        neighbors.push_back(neighbor);
        iterator.next();
    }
}

template<typename F>
void LiveGraphWrapper::Snapshot::edges(uint64_t index, F&& callback, bool logical) const {
    auto non_const_this = const_cast<Snapshot*>(this);
    if (logical) {
        index = logical2physical(index);
    }

    lg::vertex_t internal_vertex_id = logical2physical(index);
    auto iterator = non_const_this->m_transaction.get_edges(internal_vertex_id, /* label */ 0);
    
    while (iterator.valid()) {
        uint64_t neighbor = iterator.dst_id();
        // double weight = *(reinterpret_cast<const double*>(iterator.edge_data().data()));
        double weight = 1.0;
        if (logical) {
            neighbor = physical2logical(neighbor);
        }
        callback(neighbor, weight);
        iterator.next();
    }
}

std::unique_ptr<LiveGraphWrapper::Snapshot> LiveGraphWrapper::get_unique_snapshot() const {
    auto *non_const_this = const_cast<LiveGraphWrapper *>(this);
    return std::make_unique<Snapshot>(non_const_this->m_pImpl, non_const_this->m_pHashMap, m_vertex_count, m_edge_count, m_is_weighted);
}

std::shared_ptr<LiveGraphWrapper::Snapshot> LiveGraphWrapper::get_shared_snapshot() const {
    auto *non_const_this = const_cast<LiveGraphWrapper *>(this);
    return std::make_shared<Snapshot>(non_const_this->m_pImpl, non_const_this->m_pHashMap, m_vertex_count, m_edge_count, m_is_weighted);
}
