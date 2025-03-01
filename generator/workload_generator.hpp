#ifndef WORKLOAD_GENERATOR_HPP
#define WORKLOAD_GENERATOR_HPP

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <algorithm>
#include <sstream>
#include <boost/program_options.hpp>

#include "../types/types.hpp"
#include "parser.hpp"

class workloadGenerator {
private:
    std::vector<weightedEdge> m_edge_list;
    std::vector<std::vector<weightedEdge>> m_graph;
    vertexID m_num_vertices;
    std::vector<vertexID> m_degree_distribution;
    std::vector<vertexID> m_single_direct_degree_distribution;
    std::vector<uint64_t> m_prefix_sum;
    std::unordered_set<vertexID> m_high_degree_nodes;
    uint64_t m_high_degree_edge_count;
    std::unordered_set<vertexID> m_low_degree_nodes;
    uint64_t m_low_degree_edge_count;
    unsigned int m_seed;

    double m_initial_graph_ratio;
    double m_vertex_query_ratio;
    double m_edge_query_ratio;
    double m_high_degree_vertex_ratio;
    double m_high_degree_edge_ratio;
    double m_low_degree_vertex_ratio;
    double m_low_degree_edge_ratio;
    uint64_t m_uniform_based_on_degree_size;
    void load_edge_list(const std::string & input_file, bool is_weighted, char delimiter = ' ');
    void remove_duplicated_edges();
    void shuffle();
    void get_degree_distribution();
    void dump_stream(const std::string & stream_path, std::vector<operation> & stream);
    void select_high_low_degree_nodes();
    vertexID select_vertex_based_on_degree();
    void select_vertices(uint64_t target_size, std::vector<vertexID> & chosen_count, bool is_uniform = false);

    void insert_vertices_initial(const std::string & initial_stream_path);
    void insert_delete_workload(const std::string & initial_stream_path, const std::string & target_stream_path, bool is_insert, targetStreamType type);
    void update_workload(const std::string & initial_stream_path, const std::string & target_stream_path);
    void microbenchmark_query(const std::string & target_stream_path, targetStreamType ts_type, operationType op_type);
    void analytic_query_initial(const std::string & initial_stream_path);
    void analytic_query_target(const std::string & target_stream_path);
    void mixed_query_initial(const std::string & initial_stream_path);
    void mixed_query_target(const std::string & target_stream, operationType op_type);
    
public:
    workloadGenerator(Parser& parser);

    void generate_all(const std::string & dir_path);
};

#endif // WORKLOAD_GENERATOR_HPP
