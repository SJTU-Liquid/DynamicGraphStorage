#pragma once
#include <string>
#include <future>
#include <queue>
#include <vector>
#include <stdlib.h>
#include <vector>
#include <random>
#include <cassert>
#include <unordered_set>
#include <fstream>
#include <algorithm>
#include <unistd.h>
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>

#include "types/types.hpp"

#include "utils/log/log.h"
#include "utils/types.hpp"
#include "utils/config.hpp"
#include "utils/memory_cost.hpp"


void start_perf_counter(int fd) {
    ioctl(fd, PERF_EVENT_IOC_RESET, 0);
    ioctl(fd, PERF_EVENT_IOC_ENABLE, 0);
}

void stop_perf_counter(int fd) {
    ioctl(fd, PERF_EVENT_IOC_DISABLE, 0);
}

template <class W>
class EdgeDriver {
private:
    W& m_method;

    uint64_t m_timestamp;
    
    EdgeDriverConfig m_cfg;
    std::vector<uint64_t> m_dest_list;
    std::vector<std::pair<uint64_t, uint64_t>> m_search_list;
    std::vector<int> m_scan_list;
    std::vector<std::pair<uint64_t, uint64_t>> m_insert_list;
    
    std::mt19937 gen;

    void init_graph(bool is_insert_task = false);
    void init_workloads();

public:
    EdgeDriver(W& method, EdgeDriverConfig exp_cfg);

    ~EdgeDriver() {
        
    }

    void insert_test();
    void check_edge();
    void scan_neighbor();
    void execute();

    void initialize_graph_real_graph(std::vector<operation> & stream, bool is_insert = false);
    void insert_real(const std::string & target_path, const std::string & output_path);
    void microbenchmarks_real(const std::string & target_path, const std::string & output_path, operationType op_type);
    void execute_real_graph();
};

template <class W>
EdgeDriver<W>::EdgeDriver(W& method, EdgeDriverConfig exp_cfg) 
    : m_method(method), m_cfg(exp_cfg), m_timestamp(0) {}


template <class W>
void EdgeDriver<W>::init_graph(bool is_insert_task) {
    m_timestamp = 0;
    for (int i = 0; i < m_cfg.num_of_vertices; i++) {
        if (!is_insert_task) {
            init_neighbor(m_method, i, m_dest_list, i * m_cfg.neighbor_size, (i + 1) * m_cfg.neighbor_size, m_cfg);
        }
        else {
            uint64_t start = i * m_cfg.neighbor_size;
            uint64_t mid = start + m_cfg.neighbor_size * m_cfg.initial_graph_rate;
            uint64_t end = start + m_cfg.neighbor_size;
            init_neighbor(m_method, i, m_dest_list, start, mid, m_cfg);
            for (uint64_t j = mid; j < end; j++) m_insert_list.push_back({i, m_dest_list[j]});
        }
    }
    std::default_random_engine engine(m_cfg.seed);
    if(m_cfg.is_shuffle) std::shuffle(m_insert_list.begin(), m_insert_list.end(), engine);
}

template <class W>
void EdgeDriver<W>::init_workloads() {
    uint64_t num_element = m_cfg.num_of_vertices * m_cfg.neighbor_size;
    m_dest_list.reserve(num_element);

    std::mt19937_64 rng(m_cfg.seed);
    std::uniform_int_distribution<uint64_t> dist(0, m_cfg.neighbor_size * 100);
    
    for (size_t i = 0; i < m_cfg.num_of_vertices; i++) {
        std::unordered_set<uint64_t> unique_values;
        for (size_t j = 0; j < m_cfg.neighbor_size; j++) {
            uint64_t value;
            do {
                value = dist(rng);
            } while (unique_values.find(value) != unique_values.end());
            
            unique_values.insert(value);
            m_dest_list.push_back(value);
        }
    }

    std::uniform_int_distribution<uint32_t> dist_src(0, m_cfg.num_of_vertices - 1); //TODO: uint32_t to uint64_t
    std::uniform_int_distribution<uint32_t> dist_dest(0, num_element - 1);

    uint64_t num_scan = m_cfg.num_scan;
    uint64_t num_search = m_cfg.num_search;

    m_scan_list.clear();
    m_scan_list.reserve(num_scan);
    
    for (size_t i = 0; i < num_scan; i++) {
        m_scan_list.push_back(i % m_cfg.num_of_vertices);
    }

    for (size_t i = 0; i < num_search; i++) {
        uint64_t idx = dist_dest(rng);
        m_search_list.push_back({idx / m_cfg.neighbor_size, m_dest_list[idx]});
    }
}

template <class W>
void EdgeDriver<W>::insert_test() {
    auto start = std::chrono::high_resolution_clock::now();
    for (auto edge : m_insert_list) {
        insert_edge(m_method, edge.first, edge.second, m_timestamp);
    }
    auto end = std::chrono::high_resolution_clock::now();

    double duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double speed = 1000 * m_insert_list.size() / duration;

    log_info("insert duration: %.6lf", duration);
    log_info("insert speed: %.6lf meps", speed);
}

template <class W>
void EdgeDriver<W>::check_edge() {
    volatile bool flag = false;
    uint64_t sum = 0;
    auto start = std::chrono::high_resolution_clock::now();
    
    for (auto & pair : m_search_list) sum += has_edge(m_method, pair.first, pair.second, m_timestamp);

    auto end = std::chrono::high_resolution_clock::now();

    double duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double speed = 1000 * m_search_list.size() / duration;

    log_info("check edge duration: %.6lf", duration);
    log_info("check edge speed: %.6lf meps", speed);
    printf("%lu\n", sum);
}

template <class T>
void EdgeDriver<T>::scan_neighbor() {  
    uint64_t cnt = 0, ret = 0;
    auto cb = [&cnt](uint64_t dest, double weight) {
        cnt += 1;
    };  
   
    
    auto start = std::chrono::high_resolution_clock::now();

    for (auto & src : m_scan_list) {
        ret += edges(m_method, src, cb, m_timestamp);
    }

    auto end = std::chrono::high_resolution_clock::now();

    double duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double speed = 1000 * cnt / duration;


    log_info("scan duration: %.6lf", duration);
    log_info("scan speed: %.6lf meps", speed);
    printf("%lu %lu\n", cnt, ret);
    
}

template <class T>
void EdgeDriver<T>::execute() {
    auto type = m_cfg.workload_type;
    init_workloads();
    std::string output_path = m_cfg.output_dir + "/output_"  + std::to_string((int)(m_cfg.timestamp_rate * 100)) + '_' + std::to_string(container::config::BLOCK_SIZE) + '_' + std::to_string(m_cfg.neighbor_size) + '_';
    FILE *log_file;

    switch (type) {
        case operationType::INSERT:
            output_path += "insert.out"; // TODO: out or stream
            set_log_file(log_file, output_path);
            init_graph(true);
            
            insert_test();
           
            break;

        case operationType::GET_VERTEX: 
        case operationType::GET_EDGE:
        case operationType::SCAN_NEIGHBOR: 
        case operationType::GET_NEIGHBOR:
            init_graph();

            for (auto & op_type : m_cfg.mb_operation_types) {
                if (op_type == operationType::GET_EDGE) {
                    output_path += "get_edge.out";
                    log_file = fopen(output_path.c_str(), "w");
                    if (log_file == NULL) {
                        std::cerr << "unable to open file" << ' ' << output_path << std::endl;
                        exit(0);
                    }
                    log_set_fp(log_file);
                    check_edge();
                    fclose(log_file);
                }

                else if (op_type == operationType::SCAN_NEIGHBOR) {
                    output_path = m_cfg.output_dir + "/output_" + std::to_string((int)(m_cfg.timestamp_rate * 100)) + '_' + std::to_string(container::config::BLOCK_SIZE) + '_' + std::to_string(m_cfg.neighbor_size) + '_';
                    output_path += "scan_neighbor.out";

                    log_file = fopen(output_path.c_str(), "w");
                    if (log_file == NULL) {
                        std::cerr << "unable to open file" << ' ' << output_path << std::endl;
                        exit(0);
                    }
                    log_set_fp(log_file);
                    // start_perf_counter(fd_instructions);
                    // start_perf_counter(fd_cycles);
                    // start_perf_counter(fd_misses);
                    // start_perf_counter(fd_accesses);
                    // start_perf_counter(fd_L1_cache_misses);
                    // start_perf_counter(fd_L2_cache_misses);
                    // start_perf_counter(fd_LL_cache_misses);
                    // start_perf_counter(fd_L1_cache_access);
                    // start_perf_counter(fd_LL_cache_access);
                    // start_perf_counter(fd_prefetch);
                    // start_perf_counter(fd_branch_misses);
                    
                    scan_neighbor();
                    
                    // stop_perf_counter(fd_instructions);
                    // stop_perf_counter(fd_cycles);
                    // stop_perf_counter(fd_misses);
                    // stop_perf_counter(fd_accesses);
                    // stop_perf_counter(fd_L1_cache_misses);
                    // stop_perf_counter(fd_L2_cache_misses);
                    // stop_perf_counter(fd_LL_cache_misses);
                    // stop_perf_counter(fd_L1_cache_access);
                    // stop_perf_counter(fd_LL_cache_access);
                    // stop_perf_counter(fd_prefetch);
                    // stop_perf_counter(fd_branch_misses);
                    get_page();

                    fclose(log_file);
                }
            }
    }
}

template <class T>
void EdgeDriver<T>::initialize_graph_real_graph(std::vector<operation> & stream, bool is_insert) {
    auto initial_size = stream.size();
    for (uint64_t i = 0; i < initial_size; i++) {
        stream.push_back({operationType::INSERT, {stream[i].e.destination, stream[i].e.source}});
    } // TODO: directed?
    if(!is_insert) m_timestamp = m_cfg.version_rate * stream.size();
    std::default_random_engine rng(m_cfg.seed);
    if (m_cfg.is_shuffle) std::shuffle(stream.begin(), stream.end(), rng);
    init_real_graph(m_method, stream);
}

template <class T>
void EdgeDriver<T>::insert_real(const std::string & target_path, const std::string & output_path) {
    std::vector<operation> target_stream;
    read_stream(target_path, target_stream);

    auto start_global = std::chrono::high_resolution_clock::now();
    
    for (uint64_t j = 0; j < target_stream.size(); j++) {
        operation& op = target_stream[j];
        weightedEdge& edge = op.e;    
        
        insert_edge(m_method, edge.source, edge.destination, m_timestamp++);
        insert_edge(m_method, edge.destination, edge.source, m_timestamp++);
    }

    auto end_global = std::chrono::high_resolution_clock::now();
    auto duration_global = std::chrono::duration_cast<std::chrono::nanoseconds>(end_global - start_global);

    log_info("global duration: %ld", duration_global.count());
    double global_speed = (double) target_stream.size() / duration_global.count() * 1000.0;

    log_info("global speed: %.6lf", global_speed);
}


template <class T>
void EdgeDriver<T>::microbenchmarks_real(const std::string & target_path, const std::string & output_path, operationType op_type) {
    std::vector<operation> target_stream;
    read_stream(target_path, target_stream);
    if (target_stream.size() == 0) return;
    if (op_type == operationType::SCAN_NEIGHBOR) {
        auto initial_size = target_stream.size();
        int repeat_times = m_cfg.repeat_times;
        for (int i = 0; i < repeat_times; i++) {
            for (int j = 0; j < initial_size; j++) {
                target_stream.push_back(target_stream[j]);
            }
        }
    }

    auto start_global = std::chrono::high_resolution_clock::now();
    
    volatile bool flag = 1;
    uint64_t sum = 0;
    uint64_t valid_sum = 0;

    auto cb = [&sum, &valid_sum](vertexID destination, double weight) {
        sum += 1;
        valid_sum += destination;
    };
    
    for (uint64_t j = 0; j < target_stream.size(); j++) {
        operation op = target_stream[j];
        weightedEdge edge = op.e;

        if (op.type == operationType::GET_EDGE) {
            flag = has_edge(m_method, edge.source, edge.destination, m_timestamp);
            valid_sum += flag;
        }

        else if (op.type == operationType::SCAN_NEIGHBOR) {
            edges(m_method, edge.source, cb, m_timestamp);
        }
    }
    
    auto end_global = std::chrono::high_resolution_clock::now();
    auto duration_global = std::chrono::duration_cast<std::chrono::nanoseconds>(end_global - start_global);
    log_info("global duration: %ld", duration_global.count());
    double global_speed;
    if (target_stream[0].type == operationType::GET_EDGE) global_speed = (double) target_stream.size() / duration_global.count() * 1000.0;
    else global_speed = (double) sum / duration_global.count() * 1000.0;
    std::cout << valid_sum << ' ' << sum << std::endl;
    log_info("global speed: %.6lf", global_speed);
}

template <class T>
void EdgeDriver<T>::execute_real_graph() {
    auto type = m_cfg.workload_type;
    auto ts_type = m_cfg.target_stream_type;
    const auto & workload_dir = m_cfg.workload_dir;
    std::string initial_path = workload_dir + "/initial_stream_";
    std::string target_path = workload_dir + "/target_stream_";
    std::string output_path = m_cfg.output_dir + "/output_" ;
    
    FILE *log_file;
    std::vector<operation> initial_stream;

    switch (type) {
        case operationType::INSERT:
            generate_path_type(initial_path, type);
            generate_path_type(target_path, type);
            generate_path_type(output_path, type);

            generate_path_ts(initial_path, ts_type);
            generate_path_ts(target_path, ts_type);
            generate_path_ts(output_path, ts_type);

            
            log_file = fopen(output_path.c_str(), "w");
            if (log_file == NULL) {
                std::cerr << "unable to open file" << ' ' << output_path << std::endl;
                exit(0);
            }
            log_set_fp(log_file);

            read_stream(initial_path, initial_stream);
            initialize_graph_real_graph(initial_stream, true);

            insert_real(target_path, output_path);

            fclose(log_file);
            break;

        case operationType::GET_VERTEX: 
        case operationType::GET_EDGE:
        case operationType::SCAN_NEIGHBOR: 
        case operationType::GET_NEIGHBOR:
            initial_path = workload_dir + "/target_stream_insert_full.stream";
            read_stream(initial_path, initial_stream);
            initialize_graph_real_graph(initial_stream);
            for (auto & op_type : m_cfg.mb_operation_types) {
                for (auto & target_stream_type : m_cfg.mb_ts_types) {
                    ts_type = target_stream_type;
                    target_path = workload_dir + "/target_stream_";
                    output_path = m_cfg.output_dir + "/output_" ;
                    generate_path_type(target_path, op_type);
                    generate_path_type(output_path, op_type);

                    generate_path_ts(target_path, ts_type);
                    generate_path_ts(output_path, ts_type);

                    log_file = fopen(output_path.c_str(), "w");
                    if (log_file == NULL) {
                        std::cerr << "unable to open file" << ' ' << output_path << std::endl;
                        exit(0);
                    }
                    log_set_fp(log_file);

                    microbenchmarks_real(target_path, output_path, op_type);

                    fclose(log_file);
                }
            }
        break;
    }
}