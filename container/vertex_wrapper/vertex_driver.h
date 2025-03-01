#pragma once
#include <string>
#include <future>
#include <queue>
#include <vector>
#include <stdlib.h>
#include <vector>
#include <random>
#include <cassert>
#include <fstream>
#include <algorithm>

#include "types/types.hpp"

#include "utils/log/log.h"
#include "utils/types.hpp"
#include "utils/config.hpp"
// #include "ittnotify.h"

template <class W>
class VertexDriver {
private:
    W& m_method;
    uint64_t m_num_vertices;
    int m_seed;
    double m_initial_rate;
    std::string m_output_dir;

    std::vector<uint64_t> m_search_list;
    std::vector<uint64_t> m_insert_list;

    void init_vertex_table(bool is_insert_task);
    void init_workloads();

public:
    VertexDriver(W& method, uint64_t num_vertices, std::string output_dir, double initial_graph_rate = 0.8, int seed = 0)
        : m_method(method), m_num_vertices(num_vertices), m_output_dir(output_dir), m_initial_rate(initial_graph_rate), m_seed(seed) {}
    void insert_test();
    void search_test();
    void scan_test();
    void execute(bool is_insert_task);
};

template <class W>
void VertexDriver<W>::init_vertex_table(bool is_insert_task) {
    uint64_t init_num_vertices = is_insert_task ? m_initial_rate * m_num_vertices : m_num_vertices;
    for (uint64_t i = 0; i < init_num_vertices; i++) m_method.insert_vertex(i, nullptr);
    for (uint64_t i = init_num_vertices; i < m_num_vertices; i++) m_insert_list.push_back(i);
}

template <class W>
void VertexDriver<W>::init_workloads() {
    std::mt19937_64 rng(m_seed);
    std::uniform_int_distribution<uint64_t> dist_src(0, m_num_vertices - 1);

    for (uint64_t i = 0; i < m_num_vertices * 10; i++) m_search_list.push_back(dist_src(rng));
}



template <class W>
void VertexDriver<W>::insert_test() {
    auto start = std::chrono::high_resolution_clock::now();
    for (auto vertex : m_insert_list) {
        m_method.insert_vertex(vertex, nullptr);
    }
    auto end = std::chrono::high_resolution_clock::now();

    double duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double speed = 1000 * m_insert_list.size() / duration;

    log_info("insert duration: %.6lf", duration);
    log_info("insert speed: %.6lf", speed);
}


template <class W>
void VertexDriver<W>::search_test() {
    auto start = std::chrono::high_resolution_clock::now();
    uint64_t res = 0;
    for (auto vertex : m_search_list) {
        res += reinterpret_cast<uint64_t>(m_method.get_entry(vertex)); // TODO: fix this ugly code
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << res << std::endl;

    double duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double speed = 1000 * m_search_list.size() / duration;

    log_info("search duration: %.6lf", duration);
    log_info("search speed: %.6lf", speed);
}

template <class W>
void VertexDriver<W>::scan_test() {
    auto start = std::chrono::high_resolution_clock::now();
    // int res = 0;
    // for (auto vertex : m_search_list) {
    //     res += m_method.has_vertex(vertex);
    // }
    uint64_t res = 0, sum = 0;
    auto callback = [&] (uint64_t dest) {
        res += dest;
        sum += 1;
    };
    m_method.scan(callback);
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << res << std::endl;

    double duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    double speed = 1000 * sum / duration;

    log_info("scan duration: %.6lf", duration);
    log_info("scan speed: %.6lf", speed);
}

template <class W>
void VertexDriver<W>::execute(bool is_insert_task) {
    init_workloads();
    std::string output_path = m_output_dir + "/output_" + (is_insert_task ? "insert" : "search") + ".out";
    FILE *log_file;
    set_log_file(log_file, output_path);
    init_vertex_table(is_insert_task);

    if (is_insert_task) insert_test();
    else {
        search_test();
        scan_test();
    }
}