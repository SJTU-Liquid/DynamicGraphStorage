#include "workload_generator.hpp"

workloadGenerator::workloadGenerator(Parser& parser)
    : m_initial_graph_ratio(parser.get_initial_graph_ratio()), 
      m_vertex_query_ratio(parser.get_vertex_query_ratio()),
      m_edge_query_ratio(parser.get_edge_query_ratio()),
      m_high_degree_vertex_ratio(parser.get_high_degree_vertex_ratio()),
      m_high_degree_edge_ratio(parser.get_high_degree_edge_ratio()),
      m_low_degree_vertex_ratio(parser.get_low_degree_vertex_ratio()),
      m_low_degree_edge_ratio(parser.get_low_degree_edge_ratio()),
      m_seed(parser.get_seed()),
      m_uniform_based_on_degree_size(parser.get_uniform_based_on_degree_size())
{
    load_edge_list(parser.get_input_file(), parser.get_is_weighted(), parser.get_delimiter());
    remove_duplicated_edges();
    if (parser.get_is_shuffle()) shuffle();
    get_degree_distribution();
    select_high_low_degree_nodes();
}

std::string removeChar(const std::string &str, char charToRemove) {
    std::string result;
    for (char c : str) {
        if (c != charToRemove) {
            result += c;
        }
    }
    return result;
}

void split(const std::string & str, char delim, std::vector<std::string> & tokens) {
    std::stringstream ss(removeChar(str, '\r')); 
    std::string token;
    while (std::getline(ss, token, delim)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
}

void workloadGenerator::load_edge_list(const std::string & input_file, bool is_weighted, char delimiter) {
    std::ifstream handle(input_file);
    if (!handle.is_open()) {
        std::cerr << "Error: cannot open handle file" << std::endl;
        exit(1);
    }

    std::string line;

    int line_number = 1;
    std::unordered_map<vertexID, uint64_t> vertex_map;
    vertexID next_vertex_index = 0;

    while (std::getline(handle, line)) {
        if (line.empty() || line[0] == '#') {
            continue;
        }

        std::vector<std::string> tokens;
        split(line, delimiter, tokens);

        vertexID source, destination;
        double weight = random() / static_cast<double>(RAND_MAX);

        try {
            source = std::stoll(tokens.at(0));
            destination = std::stoll(tokens.at(1));
            if (is_weighted && tokens.size() > 2) {
                weight = std::stod(tokens.at(2));
            }
            if (source == destination) {
                std::cerr << "Invalid line: " << line << std::endl;
                continue;
            }

            if (vertex_map.find(source) == vertex_map.end()) {
                vertex_map[source] = next_vertex_index++;
                source = next_vertex_index - 1;
            }
            else {
                source = vertex_map[source];
            }

            if (vertex_map.find(destination) == vertex_map.end()) {
                vertex_map[destination] = next_vertex_index++;
                destination = next_vertex_index - 1;
            }
            else {
                destination = vertex_map[destination];
            }

            if (source > destination) {
                std::swap(source, destination);
            }

        } catch (const std::exception& e) {
            std::cerr << "Invalid line: " << line << " at line " << line_number << std::endl;
            continue;
        }

        m_edge_list.push_back({source, destination, weight});
        
        line_number++;
    }

    m_num_vertices = next_vertex_index;

    handle.close();

    m_graph.resize(m_num_vertices);
    for (auto edge : m_edge_list) {
        m_graph[edge.source].push_back(edge);
    }
}

void workloadGenerator::remove_duplicated_edges() {
    std::sort(m_edge_list.begin(), m_edge_list.end(), [](const weightedEdge& a, const weightedEdge& b) {
        if (a.source == b.source) {
            return a.destination < b.destination;
        } else {
            return a.source < b.source;
        }
    });
    
    uint64_t cur = 0, ahead = 0;
    for (; ahead < m_edge_list.size(); ahead++, cur++) {
        while (ahead + 1 < m_edge_list.size() && m_edge_list[ahead].source == m_edge_list[ahead + 1].source && m_edge_list[ahead].destination == m_edge_list[ahead + 1].destination) {
            ahead++;
        }
        if (ahead > cur) m_edge_list[cur] = m_edge_list[ahead];
    }
    m_edge_list.resize(cur);
}

void workloadGenerator::shuffle() {
    std::default_random_engine engine(m_seed);
    std::shuffle(m_edge_list.begin(), m_edge_list.end(), engine);
}

void workloadGenerator::get_degree_distribution() {
    m_degree_distribution.resize(m_num_vertices, 0);
    m_single_direct_degree_distribution.resize(m_num_vertices, 0);
    for (auto & e : m_edge_list) {
        m_degree_distribution[e.source]++;
        m_degree_distribution[e.destination]++;
        m_single_direct_degree_distribution[e.source]++;
    }
    uint64_t total_degree = std::accumulate(m_degree_distribution.begin(), m_degree_distribution.end(), 0);

    m_prefix_sum.resize(m_degree_distribution.size());
    uint64_t sum = 0;
    for (uint64_t i = 0; i < m_prefix_sum.size(); i++) {
        m_prefix_sum[i] = m_degree_distribution[i] + sum;
        sum = m_prefix_sum[i];
    }
}

void workloadGenerator::select_high_low_degree_nodes() {
    std::vector<std::pair<uint64_t, vertexID>> node_degrees;
    for (uint64_t i = 0; i < m_num_vertices; i++) {
        node_degrees.push_back({m_degree_distribution[i], i});
    }
    std::sort(node_degrees.begin(), node_degrees.end(), [](const std::pair<uint64_t, vertexID> & a, const std::pair<uint64_t, vertexID> & b) {
        return a.first > b.first;
    });

    uint64_t selected_high_degree_size = m_num_vertices * m_high_degree_vertex_ratio;
    std::pair<uint64_t, vertexID> node;

    m_high_degree_edge_count = 0;
    for (uint64_t i = 0; i < selected_high_degree_size; i++) {
        node = node_degrees[i];
        
        m_high_degree_nodes.insert(node.second);
        m_high_degree_edge_count += node.first;
    } 

    uint64_t selected_low_degree_size = m_num_vertices * m_low_degree_edge_ratio;

    m_low_degree_edge_count = 0;
    for (uint64_t i = 0; i < selected_low_degree_size; i++) {
        node = node_degrees[m_num_vertices - 1 - i];
        
        m_low_degree_nodes.insert(node.second);
        m_low_degree_edge_count += node.first;
    }
}

vertexID workloadGenerator::select_vertex_based_on_degree() {
    uint64_t max_value = *(m_prefix_sum.end() - 1); 
    uint64_t rand = random() % max_value;
    auto it = std::lower_bound(m_prefix_sum.begin(), m_prefix_sum.end(), rand);
    
    return it - m_prefix_sum.begin();
}

void workloadGenerator::select_vertices(uint64_t target_size, std::vector<vertexID> & chosen_count, bool is_uniform) {
    uint64_t j = 0;
    for (uint64_t i = 0; i < target_size; i++) {
        bool chosen = false;
        uint64_t vertex_chosen = 0;
        std::mt19937 gen(m_seed);
        std::uniform_int_distribution<> dis(0, m_num_vertices);
        
        while(!chosen) {
            if (is_uniform) vertex_chosen = (j++) % m_num_vertices;
            else vertex_chosen = select_vertex_based_on_degree();
            
            if (chosen_count[vertex_chosen] < m_single_direct_degree_distribution[vertex_chosen]) {
                chosen_count[vertex_chosen]++;
                chosen = true;
            }
        }
    }

    // std::unordered_map<int, int> degree_distribution;
    // for (uint64_t i = 0; i < chosen_count.size(); i++) {
    //     degree_distribution[m_degree_distribution[i]] += chosen_count[i];
    // }
    // std::vector<std::pair<int, int>> sorted_degree_distribution(degree_distribution.begin(), degree_distribution.end());
    // std::sort(sorted_degree_distribution.begin(), sorted_degree_distribution.end(),
    //           [](const std::pair<int, int>& a, const std::pair<int, int>& b) {
    //               return a.first < b.first; 
    //           });

    // for (const auto& pair : sorted_degree_distribution) {
    //     std::cout << "Degree: " << pair.first << ", Count: " << pair.second << std::endl;
    // }
}

void workloadGenerator::dump_stream(const std::string & stream_path, std::vector<operation> & stream) {
    std::ofstream file(stream_path, std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(stream.data()), stream.size() * sizeof(operation));
    }
    file.close();
}


void workloadGenerator::insert_delete_workload(const std::string & initial_stream_path, const std::string & target_stream_path, bool is_insert, targetStreamType type) {
    std::vector<operation> initial_stream;
    std::vector<operation> target_stream;
    operationType op_type = is_insert ? operationType::INSERT : operationType::DELETE;


    if (type == targetStreamType::FULL) {
        if (!is_insert) {
            for (auto & e : m_edge_list) {
                initial_stream.push_back({operationType::INSERT, e});
            }
        }
        for (auto & e : m_edge_list) {
            target_stream.push_back({op_type, e});
        }
    }

    else if (type == targetStreamType::GENERAL) {
        uint64_t initial_size = m_edge_list.size() * m_initial_graph_ratio;
        if (is_insert) {
            for (uint64_t i = 0; i < initial_size; i++) {
                initial_stream.push_back({operationType::INSERT, m_edge_list[i]});
            }
            for (uint64_t i = initial_size; i < m_edge_list.size(); i++) {
                target_stream.push_back({operationType::INSERT, m_edge_list[i]});
            }
        }

        else {
            for (auto & e : m_edge_list) {
                initial_stream.push_back({operationType::INSERT, e});
            }
            for (uint64_t i = initial_size; i < m_edge_list.size(); i++) {
                target_stream.push_back({operationType::DELETE, m_edge_list[i]});
            }
        }
    }

    else if (type == targetStreamType::HIGH_DEGREE) { 
        uint64_t target_edge_count = m_high_degree_edge_count * m_high_degree_edge_ratio;
        if (is_insert) {
            for (auto & e : m_edge_list) {
                if ((m_degree_distribution[e.source] > 512 && m_degree_distribution[e.destination] > 512) && (m_high_degree_nodes.find(e.source) != m_high_degree_nodes.end() && m_high_degree_nodes.find(e.destination) != m_high_degree_nodes.end())) {
                    target_stream.push_back({operationType::INSERT, e});
                }
                else {
                    initial_stream.push_back({operationType::INSERT, e});
                }
            }
        }

        else {
            for (auto & e : m_edge_list) {
                initial_stream.push_back({operationType::INSERT, e});
            }
            for (auto & e : m_edge_list) {
                if ((m_degree_distribution[e.source] > 512 && m_degree_distribution[e.destination] > 512) && (m_high_degree_nodes.find(e.source) != m_high_degree_nodes.end() && m_high_degree_nodes.find(e.destination) != m_high_degree_nodes.end())) {
                    target_stream.push_back({operationType::DELETE, e});
                }
            }
        }
    }

    else if (type == targetStreamType::LOW_DEGREE) {
        if (is_insert) {
            for (auto & e : m_edge_list) {
                if (m_degree_distribution[e.source] < 256 && m_degree_distribution[e.destination] < 256) {
                    target_stream.push_back({operationType::INSERT, e});
                }
                else {
                    initial_stream.push_back({operationType::INSERT, e});
                }
            }
        }

        else {
            for (auto & e : m_edge_list) {
                initial_stream.push_back({operationType::INSERT, e});
            }
            for (auto & e : m_edge_list) {
                if (m_degree_distribution[e.source] < 256 && m_degree_distribution[e.destination] < 256) {
                    target_stream.push_back({operationType::DELETE, e});
                }
            }
        }
        std::cout << initial_stream.size() << std::endl;
    }

    else if (type == targetStreamType::UNIFORM) {
        uint64_t target_size = m_uniform_based_on_degree_size;
        std::vector<vertexID> chosen_count(m_num_vertices, 0);
        select_vertices(target_size, chosen_count, true);

        for (auto & edge : m_edge_list) {
            if (chosen_count[edge.source]) {
                target_stream.push_back({op_type, edge});
                chosen_count[edge.source]--;
            } else {
                initial_stream.push_back({op_type, edge});
            }
        }

        std::cout << "selected insert size uniform: " << target_stream.size() << std::endl;
    }

    else if (type == targetStreamType::BASED_ON_DEGREE) {
        uint64_t target_size = m_uniform_based_on_degree_size;

        std::vector<vertexID> chosen_count(m_num_vertices, 0);
        select_vertices(target_size, chosen_count, false);
        for (auto & edge : m_edge_list) {
            if (chosen_count[edge.source]) {
                target_stream.push_back({operationType::INSERT, edge});
                chosen_count[edge.source]--;
            }
            
            else initial_stream.push_back({operationType::INSERT, edge});
        }

        std::cout << "selected insert size based on degree: " << target_stream.size() << std::endl;
    }

    else {
        std::cerr << "Unsupported target stream type" << std::endl;
        exit(1);
    }
    std::default_random_engine engine(m_seed);
    std::shuffle(target_stream.begin(), target_stream.end(), engine);

    dump_stream(initial_stream_path, initial_stream);
    dump_stream(target_stream_path, target_stream);
}

void workloadGenerator::update_workload(const std::string & initial_stream_path, const std::string & target_stream_path) {
    std::vector<operation> initial_stream;
    std::vector<operation> target_stream;

    uint64_t initial_size = m_edge_list.size() * m_initial_graph_ratio;

    for (uint64_t i = 0; i < initial_size; i++) {
        initial_stream.push_back({operationType::INSERT, m_edge_list[i]});
    }

    for (uint64_t j = initial_size; j < m_edge_list.size(); j++) {
        target_stream.push_back({operationType::INSERT, m_edge_list[j]});
    }

    dump_stream(initial_stream_path, initial_stream);
    dump_stream(target_stream_path, target_stream);
}

void workloadGenerator::microbenchmark_query(const std::string & target_stream_path, targetStreamType ts_type, operationType op_type) {
    std::vector<operation> target_stream;
    std::vector<vertexID> vertices(m_num_vertices);
    std::iota(vertices.begin(), vertices.end(), 0);
    std::default_random_engine engine(m_seed);
    std::shuffle(vertices.begin(), vertices.end(), engine);

    if (ts_type == targetStreamType::GENERAL) {
        if (op_type == operationType::GET_VERTEX || op_type == operationType::SCAN_NEIGHBOR || op_type == operationType::GET_NEIGHBOR) {
            vertexID target_size = m_num_vertices * m_vertex_query_ratio;

            for (uint64_t i = 0; i < target_size; i++) {
                target_stream.push_back({op_type, {vertices[i % m_num_vertices], 0, 0}});
            }
        }

        else if (op_type == operationType::GET_EDGE || op_type == operationType::GET_WEIGHT) {
            uint64_t target_size = m_edge_list.size() * m_edge_query_ratio;
            for (uint64_t i = 0; i < target_size; i++) {
                target_stream.push_back({op_type, m_edge_list[i]});
            }
        }

        else {
            std::cerr << "Unsupported operation type" << std::endl;
            exit(1);
        }
    }

    else if (ts_type == targetStreamType::HIGH_DEGREE) {
        if (op_type == operationType::GET_VERTEX || op_type == operationType::SCAN_NEIGHBOR || op_type == operationType::GET_NEIGHBOR) {
            for (uint64_t i = 0; i < m_num_vertices; i++) {
                if (m_high_degree_nodes.find(i) != m_high_degree_nodes.end() && m_degree_distribution[i] > 512) {
                    target_stream.push_back({op_type, {i, 0, 0}});
                }
            }
            std::cout << "high degree nodes" << target_stream.size() << std::endl;
        } 

        else if (op_type == operationType::GET_EDGE || op_type == operationType::GET_WEIGHT ) {
            uint64_t target_size = m_high_degree_edge_count * m_high_degree_edge_ratio;
            for (auto & e : m_edge_list) {
                if ((m_degree_distribution[e.source] > 512 && m_degree_distribution[e.destination] > 512) && (m_high_degree_nodes.find(e.source) != m_high_degree_nodes.end() && m_high_degree_nodes.find(e.destination) != m_high_degree_nodes.end())) {
                    target_stream.push_back({op_type, e});
                }
            }
            std::cout << "high degree edges" << target_stream.size() << std::endl;
        }

        else {
            std::cerr << "Unsupported operation type" << std::endl;
            exit(1);
        }
    }

    else if (ts_type == targetStreamType::LOW_DEGREE) {
        if (op_type == operationType::GET_VERTEX || op_type == operationType::SCAN_NEIGHBOR || op_type == operationType::GET_NEIGHBOR) {
            for (uint64_t i = 0; i < m_num_vertices; i++) {
                if (m_degree_distribution[i] < 256) {
                    target_stream.push_back({op_type, {i, 0, 0}});
                }
            }
            std::cout << "low degree nodes" << target_stream.size() << std::endl;
        }

        else if (op_type == operationType::GET_EDGE || op_type == operationType::GET_WEIGHT ) {
            for (auto & e : m_edge_list) {
                if (m_degree_distribution[e.source] < 256 && m_degree_distribution[e.destination] < 256) {
                    target_stream.push_back({op_type, e});
                }
            }
            std::cout << "low degree edges" << target_stream.size() << std::endl;
        }

        else {
            std::cerr << "Unsupported operation type" << std::endl;
            exit(1);
        }
    }


    else if (ts_type == targetStreamType::UNIFORM) {
        if (op_type == operationType::GET_VERTEX || op_type == operationType::SCAN_NEIGHBOR || op_type == operationType::GET_NEIGHBOR) {
            uint64_t target_size = m_uniform_based_on_degree_size;
            for (uint64_t i = 0; i < target_size; i++) {
                target_stream.push_back({op_type, {vertices[i % m_num_vertices], 0, 0}});
            }
        }
        
        else if (op_type == operationType::GET_EDGE || op_type == operationType::GET_WEIGHT) {
            uint64_t target_size = m_uniform_based_on_degree_size;
            std::vector<vertexID> chosen_count(m_num_vertices, 0);
            select_vertices(target_size, chosen_count, true);

            for (auto & edge : m_edge_list) {
                if (chosen_count[edge.source]) {
                    target_stream.push_back({op_type, edge});
                    chosen_count[edge.source]--;
                }
            }
            std::cout << "selected search size uniform: " << target_stream.size() << std::endl;
        }
        
    }

    else if (ts_type == targetStreamType::BASED_ON_DEGREE) {
        uint64_t target_size = m_uniform_based_on_degree_size;

        if (op_type == operationType::SCAN_NEIGHBOR) {
            for (uint64_t i = 0; i < target_size; i++) {
                auto src = select_vertex_based_on_degree();
                target_stream.push_back({op_type, {src, 0, 0}});
            }
        }
        else if (op_type == operationType::GET_EDGE) {
            uint64_t i = 0, src = 0;
            for (uint64_t i = 0; i < target_size; i++) {
                while(true) {
                    src = select_vertex_based_on_degree();
                    if (m_graph[src].size() != 0) break;
                }
                size_t index = std::rand() % m_graph[src].size();
                target_stream.push_back({op_type, m_graph[src][index]});
                
            }

            std::cout << "selected search size based on degree: " << target_stream.size() << std::endl;
        }
    }

    else {
        std::cerr << "Unsupported target stream type" << std::endl;
        exit(1);
    }
    std::shuffle(target_stream.begin(), target_stream.end(), engine);

    dump_stream(target_stream_path, target_stream);
}

void workloadGenerator::analytic_query_initial(const std::string & initial_stream_path) {
    std::vector<operation> initial_stream;

    for (auto & e : m_edge_list) {
        initial_stream.push_back({operationType::INSERT, e});
    }

    dump_stream(initial_stream_path, initial_stream);
}

void workloadGenerator::analytic_query_target(const std::string & target_stream_path) {
    std::vector<operation> target_stream;

    std::default_random_engine engine(m_seed);
    std::uniform_int_distribution<vertexID> vertex_distribution(0, m_num_vertices - 1);
    vertexID source = vertex_distribution(engine);

    target_stream.push_back({operationType::BFS, {source, 0, 0}});
    target_stream.push_back({operationType::PAGE_RANK, {0, 0, 0}});
    target_stream.push_back({operationType::SSSP, {source, 0, 0}});
    target_stream.push_back({operationType::TC, {0, 0, 0}});
    target_stream.push_back({operationType::WCC, {0, 0, 0}});

    dump_stream(target_stream_path, target_stream);
}

void workloadGenerator::mixed_query_initial(const std::string & initial_stream_path) {
    std::vector<operation> initial_stream;

    uint64_t initial_size = m_edge_list.size() * m_initial_graph_ratio;

    for (uint64_t i = 0; i < initial_size; i++) {
        initial_stream.push_back({operationType::INSERT, m_edge_list[i]});
    }

    dump_stream(initial_stream_path, initial_stream);
}

void workloadGenerator::mixed_query_target(const std::string & target_stream_path, operationType op_type) {
    std::vector<operation> target_stream;

    int max_threads = 32;
    
    uint64_t batch_size = (m_edge_list.size() * (1 - m_initial_graph_ratio) + max_threads - 1) / max_threads;
    std::vector<vertexID> vertices(m_num_vertices);
    std::iota(vertices.begin(), vertices.end(), 0);
    std::default_random_engine engine(m_seed);
    std::shuffle(vertices.begin(), vertices.end(), engine);
    
    uint64_t initial_size = m_edge_list.size() * m_initial_graph_ratio;
    for (int i = 0; i < max_threads; i++) {
        uint64_t start = initial_size + i * batch_size;
        uint64_t end = start + batch_size;
        if (end > m_edge_list.size()) {
            end = m_edge_list.size();
        }
        
        for (uint64_t j = start; j < end; j++) {
            target_stream.push_back({operationType::INSERT, m_edge_list[j]});
        }
        target_stream.push_back({op_type, {vertices[i % m_num_vertices], 0, 0}});
    }

    dump_stream(target_stream_path, target_stream);
}

void workloadGenerator::insert_vertices_initial(const std::string & initial_stream_path) {
    std::vector<operation> initial_stream;

    for (uint64_t i = 0; i < m_num_vertices; i++) {
        initial_stream.push_back({operationType::INSERT_VERTEX, {i, 0, 0}});
    }

    dump_stream(initial_stream_path, initial_stream);
    std::cout << "num of vertices: " << initial_stream.size() << std::endl;
    std::cout << "num of edges: " << m_edge_list.size() << std::endl;
}

void workloadGenerator::generate_all(const std::string & dir_path) {
    std::string initial_stream_path = dir_path + "/initial_stream";
    std::string target_stream_path = dir_path + "/target_stream";

    insert_vertices_initial(initial_stream_path + "_insert_vertex.stream");

    // Insert workloads
    insert_delete_workload(initial_stream_path + "_insert_full.stream", target_stream_path + "_insert_full.stream", true, targetStreamType::FULL);
    insert_delete_workload(initial_stream_path + "_insert_general.stream", target_stream_path + "_insert_general.stream", true, targetStreamType::GENERAL);
    insert_delete_workload(initial_stream_path + "_insert_uniform.stream", target_stream_path + "_insert_uniform.stream", true, targetStreamType::UNIFORM);
    insert_delete_workload(initial_stream_path + "_insert_based_on_degree.stream", target_stream_path + "_insert_based_on_degree.stream", true, targetStreamType::BASED_ON_DEGREE);
    

    // Query and analytic workloads
    analytic_query_initial(initial_stream_path + "_analytic.stream");
    microbenchmark_query(target_stream_path + "_get_edge_general.stream", targetStreamType::GENERAL, operationType::GET_EDGE);
    microbenchmark_query(target_stream_path + "_scan_neighbor_general.stream", targetStreamType::GENERAL, operationType::SCAN_NEIGHBOR);
    microbenchmark_query(target_stream_path + "_get_edge_uniform.stream", targetStreamType::UNIFORM, operationType::GET_EDGE);
    microbenchmark_query(target_stream_path + "_get_edge_based_on_degree.stream", targetStreamType::BASED_ON_DEGREE, operationType::GET_EDGE);
    microbenchmark_query(target_stream_path + "_scan_neighbor_uniform.stream", targetStreamType::UNIFORM, operationType::SCAN_NEIGHBOR);
    microbenchmark_query(target_stream_path + "_scan_neighbor_based_on_degree.stream", targetStreamType::BASED_ON_DEGREE, operationType::SCAN_NEIGHBOR);
}
