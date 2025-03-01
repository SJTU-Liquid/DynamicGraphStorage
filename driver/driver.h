#pragma once
#include <string>
#include <future>
#include <queue>
#include <vector>
#include <stdlib.h>
#include <thread>
#include <pthread.h>
#include <condition_variable>
// #include <gperftools/profiler.h>
// #include <barrier>

#include "utils/log/log.h"
#include "utils/memory_cost.hpp"
#include "algorithms/BFS.h"
#include "algorithms/SSSP.h"
#include "algorithms/PR.h"
#include "algorithms/WCC.h"

#ifdef ENABLE_ITERATOR
#include "algorithms/TC_intersect.h"
#include "algorithms/TC_opt.h"
#endif

typedef std::pair<double, vertexID> pdv;

#define DEBUG
#define COUT_DEBUG_FORCE(msg) { std::cout << __FUNCTION__ << msg << std::endl; }
#ifdef DEBUG
    #define COUT_DEBUG(msg) COUT_DEBUG_FORCE(msg)
#else
    #define COUT_DEBUG(msg)
#endif

void bind_thread_to_core(std::thread &t, int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);

    int rc = pthread_setaffinity_np(t.native_handle(), sizeof(cpu_set_t), &cpuset);
    if (rc != 0) {
        std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
    }
}

template <class F, class S>
class Driver {
private:    
    F& m_method;
    const DriverConfig& m_config;

    void initialize_graph(std::vector<operation> & stream);

    void execute_insert_delete(const std::string & target_path, const std::string & output_path);
    void execute_batch_insert(const std::string & target_path, const std::string & output_path);
    void execute_update(const std::string & target_path, const std::string & output_path, int repeat_times);
    void execute_microbenchmarks(const std::string & target_path, const std::string & output_path, operationType op_type, int num_threads);
    void execute_concurrent(std::vector<std::vector<operation>> &workload_streams);
    void execute_query();
    void execute_mixed_reader_writer(const std::string & target_path, const std::string & output_path);
    void execute_qos(const std::string & target_path_search, const std::string& target_path_scan, const std::string & output_path);
    void bfs(const S & snapshot, int thread_id, vertexID source, std::vector<vertexID> & result);
    void sssp(const S & snapshot, int thread_id, vertexID source, std::vector<double> & result);
    void wcc(const S & snapshot, int thread_id, std::vector<int> & result);
    void page_rank(const S & snapshot, int thread_id, double damping_factor, int num_iterations, std::vector<double> & result, const std::vector<uint64_t> & degree_list);

public:
    Driver(F &method, const DriverConfig & config);
    ~Driver() = default;
    void execute(operationType type, targetStreamType ts_type);
};



template <class F, class S>
Driver<F, S>::Driver(F &method, const DriverConfig & config) : m_method(method), m_config(config) {}


template <class F, class S>
void Driver<F, S>::initialize_graph(std::vector<operation> & stream) {
    wrapper::run_batch_edge_update(m_method, stream, 0, stream.size(), operationType::INSERT);
}


template <class F, class S>
void Driver<F, S>::execute_insert_delete(const std::string &target_path, const std::string &output_path) {
    std::vector<operation> target_stream;
    read_stream(target_path, target_stream);

    uint64_t num_threads = m_config.insert_delete_num_threads;
    uint64_t chunk_size = (target_stream.size() + num_threads - 1) / num_threads;
    uint64_t check_point_size = m_config.insert_delete_checkpoint_size;

    std::vector<double> thread_time(num_threads);
    std::vector<std::vector<double>> thread_check_point(num_threads);

    wrapper::set_max_threads(m_method, num_threads);

    auto start_global = std::chrono::high_resolution_clock::now();

    auto thread_function = [this, &target_stream, chunk_size, &thread_time, check_point_size, &thread_check_point](int thread_id) {
        wrapper::init_thread(m_method, thread_id);
        auto start_time = std::chrono::high_resolution_clock::now();

        uint64_t start = thread_id * chunk_size;
        uint64_t end = std::min(start + chunk_size, target_stream.size());

        for (uint64_t j = start; j < end; j++) {
            operation& op = target_stream[j];
            weightedEdge& edge = op.e;

            if (op.type == operationType::INSERT) {
                wrapper::insert_edge(m_method, edge.source, edge.destination, edge.weight);
            } else if (op.type == operationType::DELETE) {
                wrapper::remove_edge(m_method, edge.source, edge.destination);
            } else {
                throw std::runtime_error("Invalid operation type in target stream");
            }

            if ((j + 1) % check_point_size == 0) {
                auto mid_time = std::chrono::high_resolution_clock::now();
                thread_check_point[thread_id].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(mid_time - start_time).count());
            }
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        thread_time[thread_id] = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
        wrapper::end_thread(m_method, thread_id);
    };

    std::vector<std::future<void>> futures;
    for (int i = 0; i < num_threads; i++) {
        futures.push_back(std::async(std::launch::async, thread_function, i));
    }

    for (auto& future : futures) {
        future.get();
    }

    auto end_global = std::chrono::high_resolution_clock::now();
    auto duration_global = std::chrono::duration_cast<std::chrono::nanoseconds>(end_global - start_global);

    log_info("global duration: %ld", duration_global.count());
    double global_speed = static_cast<double>(target_stream.size()) / duration_global.count() * 1000000.0;

    log_info("global speed: %.6lf", global_speed);

    for (int i = 0; i < num_threads; i++) {
        log_info("thread %d took: %.6lf in total", i, thread_time[i]);
        for (size_t j = 0; j < thread_check_point[i].size(); j++) {
            log_info("thread %d check point %zu: %.6lf", i, j * check_point_size, thread_check_point[i][j]);
        }
    }
    execute_microbenchmarks(m_config.workload_dir + "/target_stream_scan_neighbor_based_on_degree.stream", m_config.output_dir + "/impact_of_scan.out", operationType::SCAN_NEIGHBOR, 1);
}


template <class F, class S>
void Driver<F, S>::execute_batch_insert(const std::string &target_path, const std::string &output_path) {
    std::vector<operation> target_stream;
    read_stream(target_path, target_stream);

    uint64_t num_threads = m_config.insert_delete_num_threads;
    // uint64_t chunk_size = (target_stream.size() + num_threads - 1) / num_threads;
    uint64_t batch_size = m_config.insert_batch_size;

    // std::atomic<uint64_t> global_batch_start{0};
    uint64_t chunk_size = (target_stream.size() + num_threads - 1) / num_threads;

    std::vector<double> thread_time(num_threads);
    std::vector<std::vector<double>> thread_check_point(num_threads);

    wrapper::set_max_threads(m_method, num_threads);

    auto start_global = std::chrono::high_resolution_clock::now();

    auto thread_function = [this, &target_stream, batch_size, chunk_size, &thread_time, &thread_check_point, &start_global](int thread_id) {
        wrapper::init_thread(m_method, thread_id);
        auto start_time = std::chrono::high_resolution_clock::now();

        uint64_t start = thread_id * chunk_size;
        uint64_t end = std::min(start + chunk_size, target_stream.size());
        uint64_t batch_end = start;

        // while ((batch_start = global_batch_start.fetch_add(batch_size)) < target_stream.size()) {
        //     batch_end = std::min(target_stream.size(), batch_start + batch_size);
        //     wrapper::run_batch_edge_update(m_method, target_stream, batch_start, batch_end, operationType::INSERT);
        //     // auto mid_time = std::chrono::high_resolution_clock::now();
        //     // thread_check_point[thread_id].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(mid_time - start_global).count());
        // }
        for (uint64_t batch_start = start; batch_start < end; batch_start = batch_end) {
            batch_end = std::min(batch_start + batch_size, end);
            wrapper::run_batch_edge_update(m_method, target_stream, batch_start, batch_end, operationType::INSERT);
            auto mid_time = std::chrono::high_resolution_clock::now();
            thread_check_point[thread_id].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(mid_time - start_global).count());
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        thread_time[thread_id] = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
        wrapper::end_thread(m_method, thread_id);
    };

    std::vector<std::future<void>> futures;
    for (int i = 0; i < num_threads; i++) {
        futures.push_back(std::async(std::launch::async, thread_function, i));
    }

    for (auto& future : futures) {
        future.get();
    }

    auto end_global = std::chrono::high_resolution_clock::now();
    auto duration_global = std::chrono::duration_cast<std::chrono::nanoseconds>(end_global - start_global);

    log_info("global duration: %ld", duration_global.count());
    double global_speed = static_cast<double>(target_stream.size()) / duration_global.count() * 1000000.0;

    log_info("global speed: %.6lf", global_speed);

    for (int i = 0; i < num_threads; i++) {
        log_info("thread %d took: %.6lf in total", i, thread_time[i]);
        for (size_t j = 0; j < thread_check_point[i].size(); j++) {
            log_info("thread %d check point %zu: %.6lf", i, j * batch_size, thread_check_point[i][j]);
        }
    }
    // execute_microbenchmarks(m_config.workload_dir + "/target_stream_scan_neighbor_based_on_degree.stream", m_config.output_dir + "/impact_of_scan.out", operationType::SCAN_NEIGHBOR, 1);
}


template <class F, class S>
void Driver<F, S>::execute_update(const std::string & target_path, const std::string & output_path, int repeat_times) {
    std::vector<operation> target_stream;
    read_stream(target_path, target_stream);

    std::vector<std::thread> threads;
    uint64_t chunk_size = (target_stream.size() + m_config.update_num_threads - 1) / m_config.update_num_threads;
    std::vector<double> thread_time(m_config.update_num_threads);

    uint64_t check_point_size = m_config.update_checkpoint_size;
    std::vector<std::vector<double>> thread_check_point(m_config.update_num_threads);

    wrapper::set_max_threads(m_method, m_config.update_num_threads);
    
    auto start_global = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < m_config.update_num_threads; i++) {
        threads.emplace_back(std::thread([this, &target_stream, chunk_size, &thread_time, repeat_times, check_point_size, &thread_check_point] (int thread_id) {
            wrapper::init_thread(m_method, thread_id);
            auto start_time = std::chrono::high_resolution_clock::now();
            uint64_t start = thread_id * chunk_size;
            uint64_t size = target_stream.size();
            uint64_t end = start + chunk_size;
            if (end > size) end = size;
            
            for (int i = 0; i < repeat_times; i++) {
                for (uint64_t j = start; j < end; j++) {
                    operation& op = target_stream[j];
                    weightedEdge& edge = op.e;
                    
                    wrapper::insert_edge(m_method, edge.source, edge.destination, edge.weight);
                    wrapper::remove_edge(m_method, edge.source, edge.destination);

                    auto mid_time = std::chrono::high_resolution_clock::now();

                    if ((j + 1) % check_point_size == 0) {
                        thread_check_point[thread_id].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(mid_time - start_time).count());
                    }
                }
            }

            auto end_time = std::chrono::high_resolution_clock::now();
            
            thread_time[thread_id] = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
            wrapper::end_thread(m_method, thread_id);
        }, i));
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto end_global = std::chrono::high_resolution_clock::now();
    auto duration_global = std::chrono::duration_cast<std::chrono::nanoseconds>(end_global - start_global);

    log_info("global duration: %d", duration_global.count());
    double global_speed = (double) target_stream.size() / duration_global.count() * 1000000.0;

    log_info("global speed: %.6lf", global_speed);

    for (int i = 0; i < m_config.update_num_threads; i++) {
        log_info("thread %d took: %.6lf in total", i, thread_time[i]);
    }

    for (int i = 0; i < m_config.update_num_threads; i++) {
        for (int j = 0; j < thread_check_point[i].size(); j++) {
            log_info("thread %d check point %d: %.6lf", i, j * check_point_size, thread_check_point[i][j]);
        }
    }

    for (uint64_t j = 0; j < target_stream.size(); j++) {
        operation op = target_stream[j];
        weightedEdge edge = op.e;
        
        wrapper::insert_edge(m_method, edge.source, edge.destination, edge.weight);
    }
    execute_microbenchmarks(m_config.workload_dir + "/target_stream_scan_neighbor_general.stream", m_config.output_dir + "/impact_of_scan.out", operationType::SCAN_NEIGHBOR, 1);
}

template <class F, class S>
void Driver<F, S>::execute_microbenchmarks(const std::string & target_path, const std::string & output_path, operationType op_type, int num_threads) {
    std::vector<operation> target_stream;
    read_stream(target_path, target_stream);
    auto initial_size = target_stream.size();
    for (int i = 0; i < m_config.repeat_times; i++) {
        for (int j = 0; j < initial_size; j++) {
            target_stream.push_back(target_stream[j]);
        }
    }
    if (target_stream.size() == 0) return;

    std::vector<std::thread> threads;
    uint64_t chunk_size = (target_stream.size() + num_threads - 1) / num_threads;
    std::vector<double> thread_time(num_threads, 0.0);
    std::vector<double> thread_speed(num_threads, 0.0);
    wrapper::set_max_threads(m_method, num_threads);

    uint64_t check_point_size = m_config.mb_checkpoint_size;
    std::vector<std::vector<double>> thread_check_point(num_threads);
    auto start = std::chrono::high_resolution_clock::now();
    auto snapshot = wrapper::get_shared_snapshot(m_method);

    std::vector<uint64_t> thread_sum(num_threads, 0);
    std::atomic<uint64_t> scan_neighbor_size(0);
    std::atomic<uint64_t> dest_sum(0);

    auto worker = [this, &target_stream, &snapshot, chunk_size, &thread_time, &thread_speed, 
        &thread_sum, &dest_sum, &thread_check_point, &op_type, check_point_size] (int thread_id) {

        uint64_t start = thread_id * chunk_size;
        uint64_t size = target_stream.size();
        uint64_t end = start + chunk_size;
        if (end > size) end = size;

        auto start_time = std::chrono::high_resolution_clock::now();
        wrapper::init_thread(m_method, thread_id);
        auto snapshot_local = wrapper::snapshot_clone(snapshot);

        uint64_t sum = 0;
        uint64_t valid_sum = 0;

        // uint64_t sum_test = 0;
        // uint64_t valid_sum_test = 0;

        auto cb = [thread_id, &sum, &valid_sum](vertexID destination, double weight) {
            valid_sum += destination;
            sum += 1;
        };

        try {
            for (uint64_t j = start; j < end; j++) {
                const operation& op = target_stream[j];
                const weightedEdge& edge = op.e;

                switch (op.type) {
                    case operationType::GET_VERTEX:
                        wrapper::snapshot_has_vertex(snapshot_local, edge.source);
                        break;
                    case operationType::GET_EDGE:
                        valid_sum += wrapper::snapshot_has_edge(snapshot_local, edge.source, edge.destination);
                        break;
                    case operationType::GET_WEIGHT:
                        wrapper::snapshot_get_weight(snapshot_local, edge.source, edge.destination);
                        break;
                    case operationType::GET_NEIGHBOR:
                        wrapper::snapshot_get_neighbors_addr(snapshot_local, edge.source);
                        break;
                    case operationType::SCAN_NEIGHBOR:
                    {
                        wrapper::snapshot_edges(snapshot_local, edge.source, cb, true);
                        // auto it = wrapper::snapshot_begin(snapshot_local, edge.source);
                        // while(it.is_valid()) {
                        //     sum_test++;
                        //     valid_sum_test += it->get_dest();
                        //     ++it;
                        // }
                    }
                        break;
                    default:
                        throw std::runtime_error("Invalid operation type in target stream\n");
                }

                if ((j + 1) % check_point_size == 0) {
                    auto mid_time = std::chrono::high_resolution_clock::now();
                    thread_check_point[thread_id].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(mid_time - start_time).count());
                }
            }
        } catch (...) {}

        auto end_time = std::chrono::high_resolution_clock::now();
        wrapper::end_thread(m_method, thread_id);

        double time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
        thread_time[thread_id] = time;

        if (op_type == operationType::GET_VERTEX || op_type == operationType::GET_EDGE || op_type == operationType::GET_WEIGHT || op_type == operationType::GET_NEIGHBOR) {
            thread_speed[thread_id] = static_cast<double>(end - start) / time * 1000000.0;
        } else if (op_type == operationType::SCAN_NEIGHBOR) {
            thread_speed[thread_id] = static_cast<double>(sum) / time * 1000000.0;
        }

        thread_sum[thread_id] = sum;
        dest_sum.fetch_add(valid_sum, std::memory_order_relaxed);
        // std::cout << sum_test << ' ' << valid_sum_test << std::endl;
    };

    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back(worker, i);
        bind_thread_to_core(threads[i], i % std::thread::hardware_concurrency());
    }

    for (auto& thread : threads) {
        thread.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    uint64_t duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    uint64_t duration_sum = 0;

    auto global_type = target_stream[0].type;
    double global_speed;
    double average_speed;

    for (int i = 0; i < num_threads; i++) {
        log_info("thread %d time: %.6lf", i, thread_time[i]);
        duration_sum += thread_time[i];
    }

    log_info("global duration: %.6lf", duration);

    if (global_type == operationType::GET_VERTEX || global_type == operationType::GET_EDGE || global_type == operationType::GET_WEIGHT || global_type == operationType::GET_NEIGHBOR) {
        for (int i = 0; i < num_threads; i++) {
            log_info("thread %d speed: %.6lf keps", i, thread_speed[i]);
        }
        global_speed = static_cast<double>(target_stream.size()) / duration * 1000000.0;
        average_speed = static_cast<double>(target_stream.size()) / duration_sum * 1000000.0;
    } else {
        for (int i = 0; i < num_threads; i++) {
            log_info("thread %d speed: %.6lf", i, thread_speed[i]);
            scan_neighbor_size.fetch_add(thread_sum[i], std::memory_order_relaxed);
        }
        global_speed = static_cast<double>(scan_neighbor_size.load()) / duration * 1000000.0;
        average_speed = static_cast<double>(scan_neighbor_size.load()) / duration_sum * 1000000.0;
    }

    log_info("global speed: %.6lf", global_speed);
    log_info("average speed: %.6lf", average_speed);

    for (int i = 0; i < num_threads; i++) {
        for (size_t j = 0; j < thread_check_point[i].size(); j++) {
            log_info("thread %d check point %d: %.6lf", i, static_cast<int>(j * check_point_size), thread_check_point[i][j]);
        }
    }

    std::cout << "dest sum: " << dest_sum.load() << " scan neighbor size: " << scan_neighbor_size.load() << std::endl;
}

template <class F, class S>
void Driver<F, S>::execute_concurrent(std::vector<std::vector<operation>> &workload_streams) {
    int num_threads = 0;
    for (int i = 0; i < m_config.concurrent_workloads.size(); i++) 
        num_threads += m_config.concurrent_workloads[i].num_threads;

    std::vector<std::thread> threads;
    std::vector<double> thread_time(num_threads);
    std::vector<double> thread_speed(num_threads);
    wrapper::set_max_threads(m_method, num_threads);

    std::vector<std::vector<double>> thread_check_point(num_threads);
    // std::barrier barrier(num_threads);

    auto snapshot = wrapper::get_shared_snapshot(m_method);

    std::vector<uint64_t> thread_sum(num_threads);
    std::atomic<uint64_t> scan_neighbor_size(0);
    std::atomic<uint64_t> dest_sum(0);
    int cur_thread_id = 0;

    auto & insert_workload = m_config.concurrent_workloads[0];
    auto & insert_stream = workload_streams[0];
    uint64_t insert_chunk_size = insert_workload.num_threads == 0 ? 0 : (insert_stream.size() + insert_workload.num_threads - 1) / insert_workload.num_threads;
    for (int i = 0; i < insert_workload.num_threads; i++) {
        threads.emplace_back(std::thread([this, &insert_chunk_size, &insert_stream, &thread_time, &thread_speed, &thread_check_point] (int thread_id) {
            wrapper::init_thread(m_method, thread_id);
            // barrier.arrive_and_wait();
            auto start_time = std::chrono::high_resolution_clock::now();
            uint64_t start = thread_id * insert_chunk_size;
            uint64_t size = insert_stream.size();
            uint64_t end = start + insert_chunk_size;
            if (end > size) end = size;
            
            for (uint64_t j = start; j < end; j++) {
                operation& op = insert_stream[j];
                auto edge = op.e;
                if (op.type == operationType::INSERT) {
                    wrapper::insert_edge(m_method, edge.source, edge.destination, edge.weight);
                }
                else {
                    throw std::runtime_error("Invalid operation type in target stream\n");
                }

                if ((j + 1) % m_config.insert_delete_checkpoint_size == 0) {
                    auto mid_time = std::chrono::high_resolution_clock::now();
                    thread_check_point[thread_id].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(mid_time - start_time).count());
                }
            }
            auto end_time = std::chrono::high_resolution_clock::now();
            
            double duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
            thread_time[thread_id] = duration;
            thread_speed[thread_id] = (double) (end - start) / thread_time[thread_id] * 1000000.0;
            wrapper::end_thread(m_method, thread_id);
        }, cur_thread_id++));
    }

    for (int reader = 1; reader < m_config.concurrent_workloads.size(); reader++) {
        auto & reader_workload = m_config.concurrent_workloads[reader];
        auto & reader_stream = workload_streams[reader];
        uint64_t reader_chunk_size = reader_workload.num_threads == 0 ? 0 : (reader_stream.size() + reader_workload.num_threads - 1) / reader_workload.num_threads;
        for (int reader_thread = 0; reader_thread < reader_workload.num_threads; reader_thread++) {
            threads.emplace_back(std::thread([this, reader_thread, &snapshot, reader_chunk_size, &reader_stream, &thread_time, &dest_sum, &thread_sum, &thread_speed, &thread_check_point] (int thread_id) {
                uint64_t start = (reader_thread) * reader_chunk_size;
                uint64_t size = reader_stream.size();
                uint64_t end = start + reader_chunk_size;
                if (end > size) end = size;

                auto start_time = std::chrono::high_resolution_clock::now();
                wrapper::init_thread(m_method, thread_id);
                // barrier.arrive_and_wait();
                auto snapshot_local = wrapper::snapshot_clone(snapshot);

                uint64_t sum = 0;
                uint64_t valid_sum = 0;

                auto cb = [thread_id, &sum, &valid_sum](vertexID destination, double weight) {
                    valid_sum += destination;
                    sum += 1;
                };

                for (uint64_t j = start; j < end; j++) {
                    const operation& op = reader_stream[j];
                    const auto& edge = op.e;

                    switch (op.type) {
                        case operationType::GET_VERTEX:
                            wrapper::snapshot_has_vertex(snapshot_local, edge.source);
                            break;
                        case operationType::GET_EDGE:
                            sum += wrapper::snapshot_has_edge(snapshot_local, edge.source, edge.destination);
                            break;
                        case operationType::GET_WEIGHT:
                            wrapper::snapshot_get_weight(snapshot_local, edge.source, edge.destination);
                            break;
                        case operationType::GET_NEIGHBOR:
                            wrapper::snapshot_get_neighbors_addr(snapshot_local, edge.source);
                            break;
                        case operationType::SCAN_NEIGHBOR:
                        {
                            wrapper::snapshot_edges(snapshot_local, edge.source, cb, true);
                        }
                            break;
                        default:
                            throw std::runtime_error("Invalid operation type in target stream\n");
                    }

                    if ((j + 1) % m_config.mb_checkpoint_size == 0) {
                        auto mid_time = std::chrono::high_resolution_clock::now();
                        thread_check_point[thread_id].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(mid_time - start_time).count());
                    }
                }


                auto end_time = std::chrono::high_resolution_clock::now();
                wrapper::end_thread(m_method, thread_id);

                double time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
                thread_time[thread_id] = time;

                auto global_type = reader_stream[0].type; // TODO: change this ugly code
                if (global_type == operationType::GET_VERTEX || global_type == operationType::GET_EDGE || global_type == operationType::GET_WEIGHT || global_type == operationType::GET_NEIGHBOR) {
                    thread_speed[thread_id] = static_cast<double>(end - start) / time * 1000000.0;
                } else if (global_type == operationType::SCAN_NEIGHBOR) {
                    thread_speed[thread_id] = static_cast<double>(sum) / time * 1000000.0;
                }

                thread_sum[thread_id] = sum;
                dest_sum.fetch_add(valid_sum, std::memory_order_relaxed);
            }, cur_thread_id++));
        }
    }

    for (auto & thread : threads) {
        thread.join();
    }

    for (int i = 0; i < num_threads; i++) {
        std::cout << i << ' ' << thread_sum[i] << std::endl;
    }

    for (int i = 0; i < num_threads; i++) {
        log_info("thread %d time: %.6lf", i, thread_time[i]);
    }
}

template <class F, class S>
void Driver<F, S>::execute_query() {
    FILE *log_file;
    
    
    for (auto &num_threads : m_config.query_num_threads) {
        for (operationType op_type : m_config.query_operation_types) {
            std::string target_path = m_config.workload_dir + "/target_stream_";
            std::string output_path = m_config.output_dir + "/output_" + std::to_string(num_threads) + "_";
            
            generate_path_type(output_path, op_type);

            log_file = fopen(output_path.c_str(), "w");
            if (log_file == NULL) {
                std::cerr << "unable to open file" << std::endl;
                exit(0);
            }
            log_set_fp(log_file);
            
            try {
                auto snapshot = wrapper::get_shared_snapshot(m_method);
                if (op_type == operationType::BFS) {
                    std::vector<std::pair<uint64_t, int64_t>> external_ids;
                    bfsExperiments<F, S> bfs(num_threads, m_config.alpha, m_config.beta, m_method, snapshot);
                    bfs.run_gapbs_bfs(m_config.bfs_source, external_ids); // TODO: add gapbs or not
                }

                else if (op_type == operationType::SSSP) {
                    std::vector<std::pair<uint64_t, double>> external_ids;
                    ssspExperiments<F, S> sssp(num_threads, m_config.delta, m_method, snapshot);
                    sssp.run_sssp(m_config.sssp_source, external_ids);
                }

                else if (op_type == operationType::PAGE_RANK) {
                    std::vector<std::pair<uint64_t, double>> external_ids;
                    pageRankExperiments<F, S> pr(num_threads, m_config.num_iterations, m_config.damping_factor, m_method, snapshot);
                    pr.run_page_rank(external_ids);
                }

                else if (op_type == operationType::WCC) {
                    std::vector<std::pair<uint64_t, int64_t>> external_ids;
                    wccExperiments<F, S> wcc(num_threads, m_method, snapshot);
                    wcc.run_wcc(external_ids);
                }

#ifdef ENABLE_ITERATOR
                else if (op_type == operationType::TC) {
                    TriangleCounting<F, S> tc(m_method, snapshot);
                    tc.run_tc();
                } 

                else if (op_type == operationType::TC_OP) {
                    TriangleCounting_optimized<F, S> tc(m_method, snapshot);
                    tc.run_tc();
                }
#endif
                else {
                    throw std::runtime_error("Invalid operation type\n");
                }
            }
            catch (std::exception & e) {
                std::cerr << e.what() << std::endl;
                std::cerr << "error" << std::endl;
            }
            fclose(log_file);
        }
    }
}

template <class F, class S>
void Driver<F, S>::bfs(const S & snapshot,int thread_id, vertexID source, std::vector<vertexID> & result) {
    std::queue<vertexID> bfs_queue;
    bfs_queue.push(source);
    bool * visited;
    visited = new bool [wrapper::size(snapshot)];
    memset(visited, 0, sizeof(bool) * wrapper::size(snapshot));
    const int INF = std::numeric_limits<int>::max();
    std::fill(result.begin(), result.end(), INF);
    result[source] = 0;

    while(!bfs_queue.empty()) {
        vertexID cur_src = bfs_queue.front();
        visited[cur_src] = true;
        bfs_queue.pop();
        vertexID level = result[cur_src] + 1;

        std::function<void(vertexID, double)> cb = [visited, level, &bfs_queue, &result](vertexID destination, double weight) {
            if(!visited[destination]) {
                visited[destination] = true;
		        bfs_queue.push(destination);
            	result[destination] = level;
	        }
        };

        wrapper::snapshot_edges(snapshot, cur_src, cb, false);
    }
    delete visited;
}

template <class F, class S>
void Driver<F, S>::sssp(const S & snapshot,int thread_id, vertexID source, std::vector<double> & result) {
    std::priority_queue<pdv, std::vector<pdv>, std::greater<pdv>> sssp_queue;
    sssp_queue.push({0, source});

    const double INF = std::numeric_limits<double>::max();
    std::fill(result.begin(), result.end(), INF);
    result[source] = 0;
    
    while(!sssp_queue.empty()) {
        vertexID cur_src = sssp_queue.top().second;
        double cur_dist = sssp_queue.top().first;
        sssp_queue.pop();

        if (cur_dist > result[cur_src]) continue;

        std::function<void(vertexID, double)> cb = [cur_src, cur_dist, &result, &sssp_queue](vertexID destination, double weight) {
            double next_dist = cur_dist + weight;

            if (next_dist < result[destination]) {
                result[destination] = next_dist;
                sssp_queue.push({next_dist, destination});
            }
        };
        
        wrapper::snapshot_edges(snapshot, cur_src, cb, true);
    }
}

class UnionFind {
public:
    std::vector<vertexID> root;

    explicit UnionFind(vertexID size) : root(size) {
        for (vertexID i = 0; i < size; i++) {
            root[i] = i;
        }
    }

    int find(vertexID x) {
        if (x == root[x]) {
            return x;
        }
        return root[x] = find(root[x]);
    }

    void unite(vertexID x, vertexID y) {
        vertexID rootX = find(x);
        vertexID rootY = find(y);
        if (rootX != rootY) {
            root[rootY] = rootX;
        }
    }
};

template <class F, class S>
void Driver<F, S>::wcc(const S & snapshot,int thread_id, std::vector<int> & result) {
    vertexID size = wrapper::snapshot_vertex_count(snapshot);
    UnionFind uf(size);

    for (vertexID source = 0; source < size; source++) {
        std::function<void(vertexID, double)> cb = [source, &uf](vertexID destination, double weight) {
            uf.unite(source, destination);
        };

        wrapper::snapshot_edges(snapshot, source, cb, false);

        result[source] = -1;
    }

    vertexID component_cnt = 0;
    for (vertexID i = 0; i < size; i++) {
        vertexID root = uf.find(i);
        if (result[root] == -1) {
            result[root] = component_cnt++;
        }
        result[i] = result[root];
    }
}

template <class F, class S>
void Driver<F, S>::page_rank(const S & snapshot, int thread_id, double damping_factor, int num_iterations, std::vector<double> & result, const std::vector<uint64_t> & degree_list) {
    uint64_t size = wrapper::snapshot_vertex_count(snapshot);       
    std::vector<double> outgoing_contrib(size);
    const double init_score = 1.0 / size;
    const double base_score = (1.0 - damping_factor) / size;
    double dangling_sum = 0.0;

    for(vertexID source = 0; source < size; source++) {
        result[source] = init_score;
    }

    for (int iter = 0; iter < num_iterations; iter++) {
        for (vertexID source = 0; source < size; source++) {
            vertexID degree = degree_list[source];
            if (degree == 0) {
                dangling_sum += result[source];
            }
            else {
                outgoing_contrib[source] = result[source] / degree;
            }
        }

        dangling_sum /= size;

        for (vertexID source = 0; source < size; source++) {
            double incoming_total = 0.0;
            auto cb = [&](uint64_t dst, double w) {
                incoming_total += outgoing_contrib[dst];
            };
            wrapper::snapshot_edges(snapshot, source, cb, false);
            
            result[source] = base_score + damping_factor * (incoming_total + dangling_sum);
            // std::cout << source << ' ' << result[source] << std::endl;
        }
    }
}


template <class F, class S>
void Driver<F, S>::execute_mixed_reader_writer(const std::string & target_path, const std::string & output_path) {
    std::vector<operation> target_stream;
    read_stream(target_path, target_stream);
    wrapper::set_max_threads(m_method, m_config.writer_threads + m_config.reader_threads + 1);

    std::vector<std::thread> writer_threads;
    uint64_t chunk_size = m_config.writer_threads == 0 ? 0 : (target_stream.size() + m_config.writer_threads - 1) / m_config.writer_threads;
    std::vector<double> thread_time(m_config.writer_threads);
    std::vector<double> thread_speed(m_config.writer_threads);
    uint64_t check_point_size = m_config.insert_delete_checkpoint_size;
    std::vector<std::vector<double>> thread_check_point(m_config.writer_threads);
    
    std::vector<std::thread> reader_threads;
    std::vector<std::vector<double>> reader_execution_time(m_config.reader_threads); 

    auto snapshot = wrapper::get_shared_snapshot(m_method);
    uint64_t num_vertices = wrapper::snapshot_vertex_count(snapshot);
    std::vector<uint64_t> degree_list(num_vertices);
    for (uint64_t i = 0; i < num_vertices; i++) degree_list[i] = wrapper::degree(m_method, i);
    bool write_ended = 0;


    auto start_global = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < m_config.writer_threads; i++) {
        writer_threads.emplace_back(std::thread([this, &target_stream, chunk_size, &thread_time, &thread_speed, check_point_size, &thread_check_point] (int thread_id) {
            wrapper::init_thread(m_method, thread_id);
            // pthread_barrier_wait(&barrier);
            auto start_time = std::chrono::high_resolution_clock::now();
            uint64_t start = thread_id * chunk_size;
            uint64_t size = target_stream.size();
            uint64_t end = start + chunk_size;
            if (end > size) end = size;
            
            for (uint64_t j = start; j < end; j++) {
                operation& op = target_stream[j];
                weightedEdge edge = op.e;
                if (op.type == operationType::INSERT) {
                    wrapper::insert_edge(m_method, edge.source, edge.destination, edge.weight);
                }
                else {
                    throw std::runtime_error("Invalid operation type in target stream\n");
                }

                if ((j + 1) % check_point_size == 0) {
                    auto mid_time = std::chrono::high_resolution_clock::now();
                    thread_check_point[thread_id].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(mid_time - start_time).count());
                }
            }
            auto end_time = std::chrono::high_resolution_clock::now();
            
            double duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
            thread_time[thread_id] = duration;
            thread_speed[thread_id] = (double) (end - start) / thread_time[thread_id] * 1000000.0;
            wrapper::end_thread(m_method, thread_id);
        }, i));
        // bind_thread_to_core(writer_threads[i], i % std::thread::hardware_concurrency());
    }

    for (int i = 0; i < m_config.reader_threads; i++) {
        reader_threads.emplace_back(std::thread([this, &reader_execution_time, &snapshot, &degree_list, &write_ended] (int thread_id) {
            // pthread_barrier_wait(&barrier);
            auto start_time = std::chrono::high_resolution_clock::now();
            wrapper::init_thread(m_method, thread_id);
            auto snapshot_local = wrapper::snapshot_clone(snapshot);
            
            do {
                auto start = std::chrono::high_resolution_clock::now();
                std::vector<double> result(wrapper::snapshot_vertex_count(snapshot_local));
                page_rank(snapshot_local, thread_id, m_config.damping_factor, m_config.num_iterations, result, degree_list);
                auto end = std::chrono::high_resolution_clock::now();
                double duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
                reader_execution_time[thread_id - m_config.writer_threads].push_back(duration);
            } while  (!write_ended);

            wrapper::end_thread(m_method, thread_id);
            auto end_time = std::chrono::high_resolution_clock::now();
            
            double duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
        }, i + m_config.writer_threads));
        bind_thread_to_core(reader_threads[i], (i + m_config.writer_threads) % std::thread::hardware_concurrency());
    }

    for (auto& thread : writer_threads) {
        thread.join();
    }
    write_ended = 1;

    for (auto& thread : reader_threads) {
        thread.join();
    }

    auto end_global = std::chrono::high_resolution_clock::now();
    auto duration_global = std::chrono::duration_cast<std::chrono::nanoseconds>(end_global - start_global);

    log_info("global duration: %ld", duration_global.count());

    double total_time = 0.0;
    for (int i = 0; i < m_config.writer_threads; i++) {
        log_info("write thread %d took: %.6lf in total", i, thread_time[i]);
        total_time += thread_time[i];
    }

    for (int i = 0; i < m_config.writer_threads; i++) {
        log_info("write thread %d speed: %.6lf in total", i, thread_speed[i]);
    }

    double global_speed = (double) target_stream.size() / duration_global.count() * 1000000.0;
    double average_speed = (double) target_stream.size() / total_time * 1000000.0;
    log_info("global speed: %.6lf", global_speed);
    log_info("average speed: %.6lf", average_speed);

    for (int i = 0; i < m_config.writer_threads; i++) {
        for (int j = 0; j < thread_check_point[i].size(); j++) {
            log_info("write thread %d check point %d: %.6lf", i, j * check_point_size, thread_check_point[i][j]);
        }
    }

    for (int i = 0; i < m_config.reader_threads; i++) {
        for (int j = 0; j < reader_execution_time[i].size(); j++) {
            log_info("read thread %d check point %d: %.6lf", i, j, reader_execution_time[i][j]);
        }
    }

}

template <class F, class S>
void Driver<F, S>::execute_qos(const std::string & target_path_search, const std::string& target_path_scan, const std::string & output_path) {
    std::vector<operation> target_stream_search;
    std::vector<operation> target_stream_scan;
    read_stream(target_path_search, target_stream_search);
    read_stream(target_path_scan, target_stream_scan);

    int num_threads_search = m_config.num_threads_search;
    int num_threads_scan = m_config.num_threads_scan;
    int num_threads = num_threads_search + num_threads_scan;

    std::vector<std::thread> threads;
    std::vector<double> thread_time(num_threads);
    std::vector<double> thread_speed(num_threads);
    wrapper::set_max_threads(m_method, m_config.num_threads_scan + m_config.num_threads_search + 1);

    uint64_t check_point_size = m_config.mb_checkpoint_size;
    std::vector<std::vector<double>> thread_check_point(num_threads);

    auto start = std::chrono::high_resolution_clock::now();
    auto snapshot = wrapper::get_shared_snapshot(m_method);

    std::vector<uint64_t> thread_sum(num_threads);
    uint64_t scan_neighbor_size = 0;
    uint64_t dest_sum = 0;

    for (int i = 0; i < num_threads_search; i++) {
        threads.push_back(std::thread([this, &target_stream_search, &snapshot, &thread_time, &thread_speed, &thread_check_point, &dest_sum, check_point_size] (int thread_id) {
            
            auto start_time = std::chrono::high_resolution_clock::now();
            wrapper::init_thread(m_method, thread_id);
            auto snapshot_local = wrapper::snapshot_clone(snapshot);
            uint64_t valid_sum = 0;

            for (uint64_t j = 0; j < target_stream_search.size(); j++) {
                operation op = target_stream_search[j];
                auto edge = op.e;
                valid_sum += wrapper::snapshot_has_edge(snapshot_local, edge.source, edge.destination);

                if ((j + 1) % check_point_size == 0) {
                    auto mid_time = std::chrono::high_resolution_clock::now();
                    thread_check_point[thread_id].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(mid_time - start_time).count());
                }
            }

            auto end_time = std::chrono::high_resolution_clock::now();
            wrapper::end_thread(m_method, thread_id);
            
            double time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
            thread_time[thread_id] = time;

            thread_speed[thread_id] = (double) target_stream_search.size() / thread_time[thread_id] * 1000000.0;
            dest_sum += valid_sum;
        }, i));
        bind_thread_to_core(threads[i], i % std::thread::hardware_concurrency());
    }

    for (int i = num_threads_search; i < num_threads_search + num_threads_scan; i++) {
        threads.push_back(std::thread([this, &target_stream_scan, &snapshot, &thread_time, &thread_speed, &thread_check_point, &dest_sum, check_point_size] (int thread_id) {
            auto start_time = std::chrono::high_resolution_clock::now();
            wrapper::init_thread(m_method, thread_id);
            auto snapshot_local = wrapper::snapshot_clone(snapshot);
            uint64_t valid_sum = 0, sum = 0;
            auto cb = [thread_id, &sum, &valid_sum](vertexID destination, double weight) {
                valid_sum += destination;
                sum += 1;
            };

            for (uint64_t j = 0; j < target_stream_scan.size(); j++) {
                operation op = target_stream_scan[j];
                auto edge = op.e;
                wrapper::snapshot_edges(snapshot_local, edge.source, cb, true);

                if ((j + 1) % check_point_size == 0) {
                    auto mid_time = std::chrono::high_resolution_clock::now();
                    thread_check_point[thread_id].push_back(std::chrono::duration_cast<std::chrono::nanoseconds>(mid_time - start_time).count());
                }
            }

            auto end_time = std::chrono::high_resolution_clock::now();
            wrapper::end_thread(m_method, thread_id);
            
            double time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
            thread_time[thread_id] = time;

            thread_speed[thread_id] = (double) sum / thread_time[thread_id] * 1000000.0;
            dest_sum += valid_sum;
        }, i));
        bind_thread_to_core(threads[i], i % std::thread::hardware_concurrency());
    }

    for (auto& thread : threads) {
        thread.join();
    }

    for (int i = 0; i < num_threads; i++) {
        log_info("thread %d time: %.6lf", i, thread_time[i]);
    }

    for (int i = 0; i < num_threads; i++) {
        for (int j = 0; j < thread_check_point[i].size(); j++) {
            log_info("thread %d check point %d: %.6lf", i, j * check_point_size, thread_check_point[i][j]);
        }
    }

    std::cout << dest_sum << std::endl;
}


template <class F, class S>
void Driver<F, S>::execute(operationType type, targetStreamType ts_type) {
    std::string vertex_path = m_config.workload_dir + "/initial_stream_insert_vertex.stream";
    // std::string initial_path = m_workload_dir + "/initial_stream_";
    // std::string target_path = m_workload_dir + "/target_stream_";
    // std::string output_path = m_output_dir + "/output_" + std::to_string(m_num_threads) + "_";
    std::string initial_path, target_path, output_path, target_path_search, target_path_scan;

    std::vector<operation> vertex_stream;
    read_stream(vertex_path, vertex_stream);
    std::vector<vertexID> vertices;
    std::transform(vertex_stream.begin(), vertex_stream.end(), std::back_inserter(vertices),
               [](const auto& op) { return op.e.source; });
    wrapper::run_batch_vertex_update(m_method, vertices, 0, vertices.size());
    
    std::vector<operation> initial_stream;
    FILE *log_file;

    if (type == operationType::QUERY) {
        initial_path = m_config.workload_dir + "/target_stream_insert_full.stream";
        read_stream(initial_path, initial_stream);
        initialize_graph(initial_stream);
        execute_query();
        return;
    }

    switch (type) {
        case operationType::INSERT: 
        case operationType::DELETE:
            initial_path = m_config.workload_dir + "/initial_stream_";
            target_path = m_config.workload_dir + "/target_stream_";
            output_path = m_config.output_dir + "/output_" + std::to_string(m_config.insert_delete_num_threads) + "_";
            generate_path_type(initial_path, type);
            generate_path_type(target_path, type);
            generate_path_type(output_path, type);

            generate_path_ts(initial_path, ts_type);
            generate_path_ts(target_path, ts_type);
            generate_path_ts(output_path, ts_type);


            log_file = fopen(output_path.c_str(), "w");
            if (log_file == NULL) {
                std::cerr << "unable to open file" << std::endl;
                exit(0);
            }
            log_set_fp(log_file);

            read_stream(initial_path, initial_stream);
            initialize_graph(initial_stream);

            execute_insert_delete(target_path, output_path);
            fclose(log_file);
            break;

        case operationType::BATCH_INSERT: 
            initial_path = m_config.workload_dir + "/initial_stream_";
            target_path = m_config.workload_dir + "/target_stream_";
            output_path = m_config.output_dir + "/output_" + std::to_string(m_config.insert_delete_num_threads) + "_" + std::to_string(m_config.insert_batch_size) + '_';
            generate_path_type(initial_path, operationType::INSERT);
            generate_path_type(target_path, operationType::INSERT); // TODO: change this ugly hard code
            generate_path_type(output_path, type);

            generate_path_ts(initial_path, ts_type);
            generate_path_ts(target_path, ts_type);
            generate_path_ts(output_path, ts_type);


            log_file = fopen(output_path.c_str(), "w");
            if (log_file == NULL) {
                std::cerr << "unable to open file" << std::endl;
                exit(0);
            }
            log_set_fp(log_file);

            read_stream(initial_path, initial_stream);
            initialize_graph(initial_stream);

            execute_batch_insert(target_path, output_path);
            fclose(log_file);
            break;
        
        
        case operationType::UPDATE:
            initial_path = m_config.workload_dir + "/initial_stream_";
            target_path = m_config.workload_dir + "/target_stream_";
            output_path = m_config.output_dir + "/output_" + std::to_string(m_config.update_num_threads) + "_";
            generate_path_type(initial_path, type);
            generate_path_type(target_path, type);
            generate_path_type(output_path, type);
            
            log_file = fopen(output_path.c_str(), "w");
            if (log_file == NULL) {
                std::cerr << "unable to open file" << std::endl;
                exit(0);
            }
            log_set_fp(log_file);

            read_stream(initial_path, initial_stream);
            initialize_graph(initial_stream);

            execute_update(target_path, output_path, m_config.update_repeat_times);
            execute_query();
            fclose(log_file);
            break;

        case operationType::GET_VERTEX: 
        case operationType::GET_EDGE:
        case operationType::GET_WEIGHT:
        case operationType::SCAN_NEIGHBOR: 
        case operationType::GET_NEIGHBOR:
            initial_path = m_config.workload_dir + "/initial_stream_insert_full.stream";
            read_stream(initial_path, initial_stream);
            initialize_graph(initial_stream);

            for (auto & operationType : m_config.mb_operation_types) {
                for (auto & num_threads : m_config.microbenchmark_num_threads) {
                    for (auto & target_type : m_config.mb_ts_types) {
                        ts_type = target_type;

                        target_path = m_config.workload_dir + "/target_stream_";
                        output_path = m_config.output_dir + "/output_" + std::to_string(num_threads) + "_";
                        generate_path_type(target_path, operationType);
                        generate_path_ts(target_path, ts_type);
                        
                        generate_path_type(output_path, operationType);
                        generate_path_ts(output_path, ts_type);

                        log_file = fopen(output_path.c_str(), "w");
                        if (log_file == NULL) {
                            std::cerr << "unable to open file" << std::endl;
                            exit(0);
                        }
                        log_set_fp(log_file);
                
                        try {
                            execute_microbenchmarks(target_path, output_path, operationType, num_threads);
                        }
                        catch (std::exception & e) {
                            std::cerr << e.what() << std::endl;
                            std::cerr << "error" << std::endl;
                        }
                        fclose(log_file);
                        
                    }
                }
            }
            break;

        case operationType::MIXED :
            initial_path = m_config.workload_dir + "/initial_stream_";
            target_path = m_config.workload_dir + "/target_stream_";
            output_path = m_config.output_dir + "/output_" + std::to_string(m_config.writer_threads) + "_" + std::to_string(m_config.reader_threads) + '_';
            generate_path_type(initial_path, operationType::INSERT);
            generate_path_type(target_path, operationType::INSERT);
            
            generate_path_ts(initial_path, targetStreamType::GENERAL);
            generate_path_ts(target_path, targetStreamType::GENERAL);

            generate_path_type(output_path, type);

            read_stream(initial_path, initial_stream);
            initialize_graph(initial_stream);

            log_file = fopen(output_path.c_str(), "w");
            if (log_file == NULL) {
                std::cerr << "unable to open file" << std::endl;
                exit(0);
            }
            log_set_fp(log_file);

            execute_mixed_reader_writer(target_path, output_path);  
            
            fclose(log_file);
            break;
        
        case operationType::QOS :
            initial_path = m_config.workload_dir + "/initial_stream_analytic.stream";
            read_stream(initial_path, initial_stream);
            initialize_graph(initial_stream);

            target_path_search = m_config.workload_dir + "/target_stream_";
            generate_path_type(target_path_search, operationType::GET_EDGE);
            generate_path_ts(target_path_search, ts_type);

            target_path_scan = m_config.workload_dir + "/target_stream_";
            generate_path_type(target_path_scan, operationType::GET_EDGE);
            generate_path_ts(target_path_scan, ts_type);

            output_path = m_config.output_dir + "/output_";
            generate_path_type(output_path, operationType::QOS);
            generate_path_ts(output_path, ts_type);
            

            log_file = fopen(output_path.c_str(), "w");
            if (log_file == NULL) {
                std::cerr << "unable to open file" << std::endl;
                exit(0);
            }
            log_set_fp(log_file);

            execute_qos(target_path_search, target_path_scan, output_path);
            
            fclose(log_file);
            break;

        case operationType::CONCURRENT:
        {
            auto insert_workload = m_config.concurrent_workloads[0];
            if (insert_workload.workload_type != operationType::INSERT) 
                throw std::runtime_error("concurrent workload must have one insert workload");

            initial_path = m_config.workload_dir + "/initial_stream_";
            output_path = m_config.output_dir + "/output_concurrent.out";
            generate_path_type(initial_path, insert_workload.workload_type);
            generate_path_ts(initial_path, insert_workload.target_stream_type);
            read_stream(initial_path, initial_stream); 
            initialize_graph(initial_stream);

            log_file = fopen(output_path.c_str(), "w");
            if (log_file == NULL) {
                std::cerr << "unable to open file" << std::endl;
                exit(0);
            }
            log_set_fp(log_file);
            int num_workloads = m_config.concurrent_workloads.size();
            std::vector<std::vector<operation>> workload_streams;
            workload_streams.resize(num_workloads);

            for (int i = 0; i < num_workloads; i++) {
                auto &workload = m_config.concurrent_workloads[i];
                target_path = m_config.workload_dir + "/target_stream_";
                generate_path_type(target_path, workload.workload_type);
                generate_path_ts(target_path, workload.target_stream_type);
                read_stream(target_path, workload_streams[i]);
            }

            execute_concurrent(workload_streams);

            fclose(log_file);
            break;
        }

            
        default:
            throw std::runtime_error("Invalid operation type\n");
    }
}
