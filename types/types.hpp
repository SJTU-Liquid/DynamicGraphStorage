#ifndef TYPES_HPP
#define TYPES_HPP
#include <memory>
#include <string>
#include <vector>
#include <fstream>

typedef uint64_t vertexID;
typedef uint8_t label;

struct weightedEdge {
    vertexID source;
    vertexID destination;
    double weight;

    weightedEdge() : source(0), destination(0), weight(0.0) {}
    weightedEdge(uint64_t source, uint64_t destination, double weight) 
        : source(source), destination(destination), weight(weight) {}
    weightedEdge(uint64_t source, uint64_t destination) 
        : source(source), destination(destination) {}
};

enum class operationType {
    // write operations
    INSERT,
    DELETE,
    INSERT_VERTEX,
    UPDATE,
    // read operations
    GET_VERTEX,
    GET_EDGE,
    GET_WEIGHT,
    SCAN_NEIGHBOR,
    GET_NEIGHBOR,
    PHYSICAL2LOGICAL,
    LOGICAL2PHYSICAL,
    // analytic operations 
    BFS,
    PAGE_RANK,
    SSSP,
    TC,
    TC_OP,
    WCC,
    
    QUERY,
    MIXED, 
    QOS, 
    CONCURRENT,
    BATCH_INSERT
};

struct operation {
    operationType type;
    weightedEdge e;
};

inline void read_stream(const std::string & stream_path, std::vector<operation> & stream) {
    std::ifstream file(stream_path, std::ios::binary);
    if (file.is_open()) {
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);

        size_t numElements = fileSize / sizeof(operation);

        stream.resize(numElements);

        file.read(reinterpret_cast<char*>(stream.data()), numElements * sizeof(operation));
    }
    file.close();
}

enum class targetStreamType {
    FULL,
    GENERAL,
    HIGH_DEGREE,
    LOW_DEGREE,
    UNIFORM,
    BASED_ON_DEGREE
};

struct concurrent_workload {
    operationType workload_type;
    targetStreamType target_stream_type;
    
    int num_threads;
};

struct Config {
    double timestamp_rate;
    int seed;
    uint64_t num_search{1000000};
    bool test_version_chain{false};
    bool enable_bloom_filter{false};

    Config(double ts_rate) : timestamp_rate(ts_rate) {}
    Config() : timestamp_rate(0.0) {}
};

struct EdgeDriverConfig {
    std::string workload_dir;
    std::string output_dir;
    operationType workload_type{operationType::INSERT};
    targetStreamType target_stream_type{targetStreamType::FULL};
    std::vector<operationType> mb_operation_types;
    std::vector<targetStreamType> mb_ts_types;

    double initial_graph_rate{0.8};
    double version_rate{0.8};

    std::vector<int> element_sizes;
    uint64_t neighbor_size{1024};
    uint64_t num_of_vertices{1024};
    bool is_shuffle{false};
    int seed;

    uint64_t num_search{1000000};
    uint64_t num_scan{1000000};
    int repeat_times{0};

    bool test_version_chain{false};
    int version_chain_length{0};
    double timestamp_rate{0.8};

    bool is_real_graph{false};
    
    explicit EdgeDriverConfig() {}
};

struct DriverConfig {
    std::string workload_dir;
    std::string output_dir;
    operationType workload_type;
    targetStreamType target_stream_type;

    // insert / delete config
    uint64_t insert_delete_checkpoint_size{10000};
    int insert_delete_num_threads{1};

    // batch insert config
    uint64_t insert_batch_size{1};

    //update config
    uint64_t update_checkpoint_size{10000};
    int update_num_threads{1};
    int update_repeat_times{10};

    // microbenchmark config
    int repeat_times{0};
    uint64_t mb_checkpoint_size{10000};
    std::vector<int> microbenchmark_num_threads;
    std::vector<operationType> mb_operation_types;
    std::vector<targetStreamType> mb_ts_types;

    // query
    std::vector<int> query_num_threads;
    std::vector<operationType> query_operation_types;

    // bfs
    int alpha{15};
    int beta{18};
    uint64_t bfs_source{0};

    // sssp
    double delta{2.0};
    uint64_t sssp_source{0};

    // pr
    int num_iterations{10};
    double damping_factor{0.85};

    // mixed
    int writer_threads{16};
    int reader_threads{16};

    //qos
    int num_threads_search{8};
    int num_threads_scan{20};

    std::vector<concurrent_workload> concurrent_workloads;
};


template <typename T>
struct Iterator {
    T iterator;
    Iterator(const T & it) : iterator(it) {}

    bool is_valid() const {
        return iterator.valid();
    }

    Iterator& operator++() {
        ++iterator;
        return (*this);
    }

    uint64_t operator*() {
        return iterator->get_dest();
    }
};


inline void generate_path_ts(std::string & path, targetStreamType ts_type) {
    if (ts_type == targetStreamType::FULL) {
        path += "full.stream";
    }
    else if (ts_type == targetStreamType::GENERAL) {
        path += "general.stream";
    }
    else if (ts_type == targetStreamType::HIGH_DEGREE) {
        path += "high_degree.stream";
    }
    else if (ts_type == targetStreamType::LOW_DEGREE) {
        path += "low_degree.stream";
    }
    else if (ts_type == targetStreamType::UNIFORM) {
        path += "uniform.stream";
    }
    else if (ts_type == targetStreamType::BASED_ON_DEGREE) {
        path += "based_on_degree.stream";
    }
    else {
        throw std::runtime_error("Invalid target stream type\n");
    }
}

inline void generate_path_type(std::string & path, operationType type) {
    if (type == operationType::INSERT) {
        path += "insert_";
    }
    else if (type == operationType::BATCH_INSERT) {
        path += "batch_insert_";
    }
    else if (type == operationType::DELETE) {
        path += "delete_";
    }
    else if (type == operationType::UPDATE) {
        path += "update.stream";
    }
    else if (type == operationType::GET_VERTEX) {
        path += "get_vertex_";
    }
    else if (type == operationType::GET_EDGE) {
        path += "get_edge_";
    }
    else if (type == operationType::GET_WEIGHT) {
        path += "get_weight_";
    }
    else if (type == operationType::SCAN_NEIGHBOR) {
        path += "scan_neighbor_";
    }
    else if (type == operationType::GET_NEIGHBOR) {
        path += "get_neighbor_";
    }
    else if (type == operationType::BFS) {
        path += "bfs.stream";
    }
    else if (type == operationType::SSSP) {
        path += "sssp.stream";
    }
    else if (type == operationType::PAGE_RANK) {
        path += "page_rank.stream";
    }
    else if (type == operationType::WCC) {
        path += "wcc.stream";
    }
    else if (type == operationType::TC) {
        path += "tc.stream";
    }
    else if (type == operationType::TC_OP) {
        path += "tc_op.stream";
    }
    else if (type == operationType::MIXED) {
        path += "mixed.stream";
    }
    else if (type == operationType::QOS) {
        path += "qos_";
    }
    else {
        throw std::runtime_error("Invalid operation type\n");
    }
}
#endif // TYPES_HPP
