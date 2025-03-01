#include "avl2pam_cow.h"

#include <utility>
#include "wrapper.h"
#include "driver.h"

// Functions for debug purpose
namespace wrapper {
    void wrapper_test() {
        auto wrapper = AVLTree_PAMTree_Wrapper();
        std::cout << "This is " << wrapper_repl(wrapper) << "!" << std::endl;
        for(int i = 0; i < 10000; i++) {
            insert_vertex(wrapper, i);
        }
        // in place: about 6000us
        // copy-on-write: about 21000us
        auto start = std::chrono::high_resolution_clock::now();
        insert_edge(wrapper, 0, 1);
        insert_edge(wrapper, 0, 5);
        for(int i = 1; i < 256; i += 2) {
            insert_edge(wrapper, 1, i);
            insert_edge(wrapper, 2, i);
            insert_edge(wrapper, 5, i);
        }
        // for (int i = 1; i < 255; i++) {
        //     insert_edge(wrapper, 6, i);
        // }
        // insert_edge(wrapper, 0, 1);
        // insert_edge(wrapper, 0, 2);
        // insert_edge(wrapper, 0, 3);
        // insert_edge(wrapper, 0, 4);
        // insert_edge(wrapper, 0, 5);
        // insert_edge(wrapper, 1, 0);


        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "Insert Time: " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << "us" << std::endl;
        std::cout << wrapper.intersect(1, 5) << std::endl;
        assert(vertex_count(wrapper) == 10000);
        assert(is_empty(wrapper) == false);
        assert(has_vertex(wrapper, 0) == true);
        assert(has_edge(wrapper, 0, 1) == true);
        std::cout << wrapper_repl(wrapper) << " basic tests passed!" << std::endl;
        auto unique_snapshot = get_unique_snapshot(wrapper);
        auto cloned_snapshot = snapshot_clone(unique_snapshot);
        assert(snapshot_vertex_count(unique_snapshot) == 10000);
        assert(snapshot_vertex_count(cloned_snapshot) == 10000);
        std::vector<uint64_t> neighbor;
        uint64_t sum = 0;
//        for(int j = 0; j < 100; j++) {
//            for (int i = 0; i < 10000; i++) {
//                if(i != j) {
//                    sum += has_edge(wrapper, i, j);
//                }
//            }
//        }
        snapshot_edges(unique_snapshot, 1, [&](uint64_t dest, double weight) {
            sum += dest;
        }, false);
        std::cout << "Sum of neighbors of 1: " << sum << std::endl;
        std::cout << snapshot_degree(unique_snapshot, 2) << std::endl;
        std::cout << wrapper_repl(wrapper) << " snapshot tests passed!" << std::endl;
        std::cout << wrapper_repl(wrapper) << " tests passed!" << std::endl;

        // origin: 780-800ms
        // non-version: 710-730ms
        std::cout << "BFS Test" << std::endl;
        // start timer
        start = std::chrono::high_resolution_clock::now();
        for(int i = 0; i < 1000; i++) {
            std::vector<char> visit(10000);
            std::queue<uint64_t> q;
            q.push(0);
            while (!q.empty()) {
                auto vertex = q.front();
                q.pop();
                if (visit[vertex]) {
                    continue;
                }
                visit[vertex] = 1;
                snapshot_edges(unique_snapshot, vertex, [&](uint64_t dest, double weight) {
                    q.push(dest);
                }, false);
            }
        }
        // end timer
        end = std::chrono::high_resolution_clock::now();
        std::cout << "BFS Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms" << std::endl;
    }

   void execute(const DriverConfig & config) {
       auto wrapper = AVLTree_PAMTree_Wrapper(false, true);
       Driver<AVLTree_PAMTree_Wrapper, std::shared_ptr<AVLTree_PAMTree_Wrapper::Snapshot>> d(wrapper, config);
       d.execute(config.workload_type, config.target_stream_type);
        // wrapper_test();
   }
}

#include "driver_main.h"

// Function Implementations

// Init
//void AVLTree_PAMTree_Wrapper::load(const std::string &path, driver::reader::readerType type) {
//    auto reader = driver::reader::Reader::open(path, type);
//    switch (type) {
//        case driver::reader::readerType::edgeList: {
//            weightedEdge edge;
//            while (reader->read(edge)) {
//                try {
//                    if (!has_vertex(edge.source)) {
//                        insert_vertex(edge.source);
//                    }
//                    if (!has_vertex(edge.destination)) {
//                        insert_vertex(edge.destination);
//                    }
//                    if (!has_edge(edge.source, edge.destination)) {
//                        insert_edge(edge.source, edge.destination);
//                    }
//                } catch (const std::exception & e) {
//                    std::cerr << e.what() << std::endl;
//                }
//            }
//            break;
//        }
//        case driver::reader::readerType::vertexList: {
//            uint64_t vertex;
//            while (reader->read(vertex)) {
//                try {
//                    if (!has_vertex(vertex)) {
//                        insert_vertex(vertex);
//                    }
//                } catch (const std::exception & e) {
//                    std::cerr << e.what() << std::endl;
//                }
//            }
//            break;
//        }
//    }
//}
std::shared_ptr<AVLTree_PAMTree_Wrapper> AVLTree_PAMTree_Wrapper::create_update_interface(const std::string& graph_type) {
    if (graph_type == "vec2vec") {
        return std::make_shared<AVLTree_PAMTree_Wrapper>();
    }
    else {
        throw std::runtime_error("Graph name not recognized, interfaces::create_update_interface");
    }
}

// Graph Operations
bool AVLTree_PAMTree_Wrapper::is_directed() const {
    return m_is_directed;
}

bool AVLTree_PAMTree_Wrapper::is_weighted() const {
    return m_is_weighted;
}

bool AVLTree_PAMTree_Wrapper::is_empty() const {
    return (this->vertex_count() == 0);
}

bool AVLTree_PAMTree_Wrapper::has_vertex(uint64_t vertex) const {
    auto tx = tm.get_read_transaction();
    auto has_vertex = tx.has_vertex(vertex);
    tx.commit();
    return has_vertex;
}

bool AVLTree_PAMTree_Wrapper::has_edge(weightedEdge edge) const {
    return has_edge(edge.source, edge.destination, edge.weight);
}

bool AVLTree_PAMTree_Wrapper::has_edge(uint64_t source, uint64_t destination) const {
    auto tx = tm.get_read_transaction();
    auto has_edge = tx.has_edge(source, destination);
    tx.commit();
    return has_edge;
}

bool AVLTree_PAMTree_Wrapper::has_edge(uint64_t source, uint64_t destination, double weight) const {
    throw driver::error::FunctionNotImplementedError("has_edge::weighted");
}

uint64_t AVLTree_PAMTree_Wrapper::intersect(uint64_t vtx_a, uint64_t vtx_b) const {
    auto tx = tm.get_read_transaction();
    auto res = tx.intersect(vtx_a, vtx_b);
    return res;
}

uint64_t AVLTree_PAMTree_Wrapper::degree(uint64_t vertex) const {
    auto tx = tm.get_read_transaction();
    if (!tx.has_vertex(vertex)) {
        throw driver::error::GraphLogicalError("Vertex does not exist : AVLTree_PAMTree_Wrapper::degree");
    }
    auto degree = tx.get_degree(vertex);
    tx.commit();
    return degree;
}

double AVLTree_PAMTree_Wrapper::get_weight(uint64_t source, uint64_t destination) const {
    throw driver::error::FunctionNotImplementedError("get_weight");
}

uint64_t AVLTree_PAMTree_Wrapper::logical2physical(uint64_t vertex) const {
    return vertex;
}

uint64_t AVLTree_PAMTree_Wrapper::physical2logical(uint64_t physical) const {
    return physical;
}

uint64_t AVLTree_PAMTree_Wrapper::vertex_count() const {
    auto tx = tm.get_read_transaction();
    auto num_vertices = tx.container_impl->vertex_count();
    tx.commit();
    return num_vertices;
}

uint64_t AVLTree_PAMTree_Wrapper::edge_count() const {
    auto tx = tm.get_read_transaction();
    auto num_edges = tx.container_impl->edge_count();
    tx.commit();
    return num_edges;
}

void AVLTree_PAMTree_Wrapper::get_neighbors(uint64_t vertex, std::vector<uint64_t> &) const {
    throw driver::error::FunctionNotImplementedError("get_neighbors");
}

void AVLTree_PAMTree_Wrapper::get_neighbors(uint64_t vertex, std::vector<std::pair<uint64_t, double>> &) const {
    throw driver::error::FunctionNotImplementedError("get_neighbors");
}

bool AVLTree_PAMTree_Wrapper::insert_vertex(uint64_t vertex) {
    auto tx = tm.get_write_transaction();
    bool inserted = true;
    try {
        tx.insert_vertex(vertex);
        tx.commit();
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        tx.abort();
        inserted = false;
    }
    return inserted;
}

bool AVLTree_PAMTree_Wrapper::insert_edge(uint64_t source, uint64_t destination, double weight) {
    auto tx = tm.get_write_transaction();
    bool inserted = true;
    try {
        tx.insert_edge(source, destination);
        if (!m_is_directed) {
            tx.insert_edge(destination, source);
        }
        inserted = tx.commit();
    } catch (std::exception &e) {
        std::cerr << e.what() << std::endl;
        tx.abort();
        inserted = false;
    }
    return inserted;
}

bool AVLTree_PAMTree_Wrapper::insert_edge(uint64_t source, uint64_t destination) {
    return insert_edge(source, destination, 0);
}

bool AVLTree_PAMTree_Wrapper::remove_vertex(uint64_t vertex) {
    throw driver::error::FunctionNotImplementedError("remove_vertex");
}

bool AVLTree_PAMTree_Wrapper::remove_edge(uint64_t source, uint64_t destination) {
    throw driver::error::FunctionNotImplementedError("remove_edge");
}

bool AVLTree_PAMTree_Wrapper::run_batch_vertex_update(std::vector<uint64_t> &vertices, uint64_t start, uint64_t end) {
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

bool AVLTree_PAMTree_Wrapper::run_batch_edge_update(std::vector<operation> & edges, uint64_t start, uint64_t end, operationType type) {
    switch (type) {
        case operationType::INSERT: {
            auto tx = tm.get_write_transaction();
            try {
                tx.insert_edge_batch(edges, start, end);
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

void AVLTree_PAMTree_Wrapper::clear() {

}

// Snapshot Related Function Implementations
std::unique_ptr<AVLTree_PAMTree_Wrapper::Snapshot> AVLTree_PAMTree_Wrapper::get_unique_snapshot() const {
    Snapshot tmp(tm);
    return std::make_unique<Snapshot>(std::move(tmp));
}

std::shared_ptr<AVLTree_PAMTree_Wrapper::Snapshot> AVLTree_PAMTree_Wrapper::get_shared_snapshot() const {
    Snapshot tmp(tm);
    return std::make_shared<Snapshot>(std::move(tmp));
}


uint64_t AVLTree_PAMTree_Wrapper::Snapshot::size() const {
    return this->m_num_edges;
}

uint64_t AVLTree_PAMTree_Wrapper::Snapshot::physical2logical(uint64_t physical) const {
    return physical;
}

uint64_t AVLTree_PAMTree_Wrapper::Snapshot::logical2physical(uint64_t logical) const {
    return logical;
}

uint64_t AVLTree_PAMTree_Wrapper::Snapshot::degree(uint64_t vertex, bool logical) const {
#ifdef ENABLE_FLAT_SNAPSHOT
    if (vertex >= flat_snapshot.size()) {
        throw driver::error::GraphLogicalError("Vertex does not exist : AVLTree_PAMTree_Wrapper::snapshot::degree");
    }
    return flat_snapshot[vertex].degree;
#else
    return m_transaction.get_degree(vertex);
#endif
}

bool AVLTree_PAMTree_Wrapper::Snapshot::has_vertex(uint64_t vertex) const {
    auto non_const_this = const_cast<Snapshot*>(this);
    return non_const_this->m_transaction.has_vertex(vertex);
}

bool AVLTree_PAMTree_Wrapper::Snapshot::has_edge(weightedEdge edge) const {
    return has_edge(edge.source, edge.destination);
}

bool AVLTree_PAMTree_Wrapper::Snapshot::has_edge(uint64_t source, uint64_t dest) const {
#ifdef ENABLE_FLAT_SNAPSHOT
    auto pam_tree = flat_snapshot[source].tree;
#else
    auto pam_struct = vertex_root.find(source);
    if (!pam_struct.has_value()) return false;
    pam_map<container::PAMEntry> pam_tree = pam_struct.value().tree;
#endif

    auto pam_entry = pam_tree.find_approx(container::get_lowest_key(dest));

    if (pam_entry.has_value()) {
        std::vector<uint64_t>& vec_ptr = pam_entry.value();
        auto pos = std::lower_bound(vec_ptr.begin(), vec_ptr.end(), dest);
        return (pos != vec_ptr.end() && *pos == dest);
    }
    return false;
}

bool AVLTree_PAMTree_Wrapper::Snapshot::has_edge(uint64_t source, uint64_t destination, double weight) const {
    throw driver::error::FunctionNotImplementedError("snapshot::has_edge::weighted");
}

uint64_t AVLTree_PAMTree_Wrapper::Snapshot::intersect(uint64_t vtx_a, uint64_t vtx_b) const {
    return m_transaction.intersect(vtx_a, vtx_b);
}

double AVLTree_PAMTree_Wrapper::Snapshot::get_weight(uint64_t source, uint64_t destination) const {
    throw driver::error::FunctionNotImplementedError("snapshot::get_weight");
}

uint64_t AVLTree_PAMTree_Wrapper::Snapshot::vertex_count() const {
    return this->m_num_vertices;
}

uint64_t AVLTree_PAMTree_Wrapper::Snapshot::edge_count() const {
    return this->m_num_edges;
}

auto AVLTree_PAMTree_Wrapper::Snapshot::begin(uint64_t src) const {
    return m_transaction.begin(src);
}

void AVLTree_PAMTree_Wrapper::Snapshot::get_neighbor_addr(uint64_t index) const {
    // m_transaction.get_neighbor(index);
}

void AVLTree_PAMTree_Wrapper::Snapshot::edges(uint64_t index, std::vector<uint64_t> &neighbors, bool logical) const {
    auto cb = [&neighbors] (uint64_t dest, double weight) {
        neighbors.push_back(dest);
        return true; // TODO: change this
    };

    edges(index, cb, logical);
    std::sort(neighbors.begin(), neighbors.end());
}

template<typename F>
uint64_t AVLTree_PAMTree_Wrapper::Snapshot::edges(uint64_t index, F&& callback, bool logical) const {
#ifdef ENABLE_FLAT_SNAPSHOT
    auto pam_tree = flat_snapshot[index].tree;
#else
    auto pam_struct = vertex_root.find(index);
    if (!pam_struct.has_value()) return false;
    pam_map<container::PAMEntry> pam_tree = pam_struct.value().tree;
#endif

    uint64_t sum = 0;
    bool flag = true;
    auto edge_map_func = [&] (PAMTree::E& l) -> void {
        if (!flag) return;
        for(size_t idx = 0; idx < l.second.size(); idx++) {
            auto dest = l.second[idx];
            bool should_continue = callback(dest, 0.0);
            sum += dest;
            if (!should_continue) {
                flag = false;
                return;
            }
        }
    };
    
    PAMTree::foreach_seq(pam_tree, edge_map_func);
    return sum;
}