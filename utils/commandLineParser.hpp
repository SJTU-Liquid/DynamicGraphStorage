#ifndef COMMAND_LINE_PARSER_HPP
#define COMMAND_LINE_PARSER_HPP
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <memory>
#include <boost/program_options.hpp>
#include "types.hpp"

class commandLineParser {
public:
    static commandLineParser& get_instance() {
        static commandLineParser instance;
        return instance;
    }

    void parse(const std::string& configFilePath);
    std::string get_workload_dir();
    std::string get_output_dir();
    int get_num_threads();
    int get_seed();
    uint64_t get_num_vertices();
    operationType get_workload_type();
    targetStreamType get_target_stream_type();
    
    uint64_t get_insert_delete_checkpoint_size();
    int get_insert_delete_num_threads();
    int get_update_num_threads();
    int get_repeat_times();
    uint64_t get_mb_checkpoint_size();
    std::vector<int> get_microbenchmark_num_threads();
    std::vector<int> get_query_num_threads(); 
    int get_alpha();
    int get_beta();
    uint64_t get_bfs_source();
    double get_delta();
    uint64_t get_sssp_source();
    int get_num_iterations();
    double get_damping_factor();
    int get_writer_threads();
    int get_reader_threads();
    int get_num_threads_search();
    int get_num_threads_scan();
    DriverConfig get_driver_config();

    std::vector<int>&& get_element_sizes();
    bool get_real_graph();
    double get_initial_graph_rate();
    double get_timestamp_rate();
    uint64_t get_num_search();
    uint64_t get_num_scan();
    int get_neighbor_test_repeat_times();
    bool get_test_version_chain();
    EdgeDriverConfig get_edge_driver_config();

    Config get_exp_config();
    
private:
    int m_num_threads;
    int m_seed;
    
    std::string m_workload_dir;
    std::string m_output_dir;
    operationType m_workload_type;
    targetStreamType m_target_stream_type;

    uint64_t insert_delete_checkpoint_size{10000};
    int insert_delete_num_threads{1};
    uint64_t insert_batch_size{1};

    uint64_t update_checkpoint_size{10000};
    int update_num_threads{1};
    int update_repeat_times{10};

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

    // concurrent
    std::vector<concurrent_workload> concurrent_workloads;

    // neighbor set test
    std::vector<int> element_sizes;
    uint64_t neighbor_size;
    uint64_t num_of_vertices;
    uint64_t m_num_vertices;
    double timestamp_rate{0.8};
    double initial_graph_rate{0.8};
    double version_rate{0.8};
    uint64_t num_search{1000000};
    uint64_t num_scan{1000000};
    int neighbor_test_repeat_times{0};
    bool test_version_chain{false};
    int version_chain_length{0};
    bool is_real_graph{false};
    bool is_shuffle{false};
};

#endif // COMMAND_LINE_PARSER_HPP