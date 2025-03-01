#include "commandLineParser.hpp"

namespace po = boost::program_options;

operationType get_op_type(std::string workload_type) {
    if (workload_type == "insert") {
        return operationType::INSERT;
    } else if (workload_type == "batch_insert") {
        return operationType::BATCH_INSERT;
    } else if (workload_type == "delete") {
        return operationType::DELETE;
    } else if (workload_type == "update") {
        return operationType::UPDATE;
    } else if (workload_type == "micro_benchmark") {
        return operationType::GET_VERTEX;
    } else if (workload_type == "bfs") {
        return operationType::BFS;
    } else if (workload_type == "sssp") {
        return operationType::SSSP;
    } else if (workload_type == "pr") {
        return operationType::PAGE_RANK;
    } else if (workload_type == "wcc") {
        return operationType::WCC;
    } else if (workload_type == "tc") {
        return operationType::TC;
    } else if (workload_type == "tc_op") {
        return operationType::TC_OP;
    } else if (workload_type == "query") {
        return operationType::QUERY;
    } else if (workload_type == "mixed") {
        return operationType::MIXED;
    } else if (workload_type == "qos") {
        return operationType::QOS;
    } else if (workload_type == "get_vertex") {
        return operationType::GET_VERTEX;
    } else if (workload_type == "get_weight") {
        return operationType::GET_WEIGHT;
    } else if (workload_type == "get_edge") {
        return operationType::GET_EDGE;
    } else if (workload_type == "scan_neighbor") {
        return operationType::SCAN_NEIGHBOR;
    } else if (workload_type == "get_neighbor") {
        return operationType::GET_NEIGHBOR;
    } else if (workload_type == "concurrent") {
        return operationType::CONCURRENT;
    } else {
        std::cerr << "no matching operation type" << std::endl;
        exit(0);
    }
}

targetStreamType get_ts_type(std::string target_stream_type) {
    if (target_stream_type == "full") {
        return targetStreamType::FULL;
    } else if (target_stream_type == "general") {
        return targetStreamType::GENERAL;
    } else if (target_stream_type == "high_degree") {
        return targetStreamType::HIGH_DEGREE;
    } else if (target_stream_type == "low_degree") {
        return targetStreamType::LOW_DEGREE;
    } else if (target_stream_type == "uniform") {
        return targetStreamType::UNIFORM;
    } else if (target_stream_type == "based_on_degree") {
        return targetStreamType::BASED_ON_DEGREE;
    } else {
        std::cerr << "no matching target stream type" << std::endl;
        exit(0);
    }
}

void commandLineParser::parse(const std::string& configFilePath) {
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help, h", "produce help message")
        ("workload_dir, d", po::value<std::string>(), "workload directory")
        ("output_dir, o", po::value<std::string>(), "output directory")
        ("num_threads, n", po::value<int>(), "number of threads")
        ("seed, s", po::value<int>(), "seed")
        ("workload_type, w", po::value<std::string>(), "type of workloads")
        ("target_stream_type, t", po::value<std::string>(), "type of target stream")
        
        ("insert_delete_checkpoint_size", po::value<uint64_t>(), "insert delete checkpoint size")
        ("insert_delete_num_threads", po::value<int>(), "number of threads for insert/delete operations")
        ("insert_batch_size", po::value<uint64_t>(), "batch size for batch insert test")

        ("update_checkpoint_size", po::value<uint64_t>(), "update checkpoint size")
        ("update_num_threads", po::value<int>(), "number of threads for update operations")
        ("update_repeat_times", po::value<int>(), "number of repeat times of update")

        ("mb_repeat_times", po::value<int>(), "number of micro_benchmark repeat times")
        ("mb_checkpoint_size", po::value<uint64_t>(), "microbenchmark checkpoint size")
        ("microbenchmark_num_threads", po::value<std::vector<int>>()->multitoken(), "number of threads for microbenchmark")
        ("mb_operation_types", po::value<std::vector<std::string>>()->multitoken(), "operation types of micro_benchmark")
        ("mb_ts_types", po::value<std::vector<std::string>>()->multitoken(), "target stream types of micro_benchmark")

        ("query_num_threads", po::value<std::vector<int>>()->multitoken(), "number of threads for query operations")
        ("query_operation_types", po::value<std::vector<std::string>>()->multitoken(), "target stream types of analytic queries")

        ("alpha", po::value<int>(), "alpha parameter for bfs")
        ("beta", po::value<int>(), "beta parameter for bfs")
        ("bfs_source", po::value<uint64_t>(), "source vertex for bfs")
        ("delta", po::value<double>(), "delta parameter for sssp")
        ("sssp_source", po::value<uint64_t>(), "source vertex for sssp")
        ("num_iterations", po::value<int>(), "number of iterations for pr")
        ("damping_factor", po::value<double>(), "damping factor for pr")

        ("writer_threads", po::value<int>(), "number of writer threads for mixed workload")
        ("reader_threads", po::value<int>(), "number of reader threads for mixed workload")

        ("num_threads_search", po::value<int>(), "number of threads for search operations in qos")
        ("num_threads_scan", po::value<int>(), "number of threads for scan operations in qos")

        ("concurrent_workloads", po::value<std::vector<std::string>>()->multitoken(), "concurrent workload parameters in the format workload_type=type target_stream_type=stream num_threads=4")

        
        ("element_sizes, e", po::value<std::vector<int>>(&element_sizes)->multitoken(), "Enter a list of integers")
        ("neighbor_size", po::value<uint64_t>(), "number of elements of each neighborhood used in synthetic graph")
        ("num_of_vertices", po::value<uint64_t>(), "number of vertices used in synthetic graph")
        ("is_shuffle", po::value<bool>(), "shuffle insert stream used in synthetic graph")
        ("real_graph, r", po::value<bool>(), "is real graph")
        ("initial_graph_rate", po::value<double>())
        ("timestamp_rate", po::value<double>(), "timestamp_rate")
        ("num_vertices", po::value<uint64_t>(), "num of vertices")
        ("num_search", po::value<uint64_t>(), "number of search operations")
        ("num_scan", po::value<uint64_t>(), "number of scan operations")
        ("neighbor_test_repeat_times", po::value<int>(), "number of neighbor test repeat times")
        ("test_version_chain", po::value<bool>(), "test version chain flag")
        ("version_chain_length", po::value<int>(), "length of version chain")
        ("version_rate", po::value<double>(), "version used when testing")
    ;

    std::ifstream configFile(configFilePath);
    if (!configFile) {
        std::cout << "failed to open config file\n";
        exit(1);
    }

    po::variables_map vm;
    po::store(po::parse_config_file(configFile, desc), vm);
    po::notify(vm);    

    if (vm.count("help")) {
        std::cout << desc << "\n";
        exit(0);
    }

    if (vm.count("workload_dir")) {
        m_workload_dir = vm["workload_dir"].as<std::string>();
    } else {
        std::cout << "Workload directory was not set.\n";
    }

    if (vm.count("output_dir")) {
        m_output_dir = vm["output_dir"].as<std::string>();
    } else {
        std::cout << "Output directory was not set.\n";
    }

    if (vm.count("insert_delete_checkpoint_size")) {
        insert_delete_checkpoint_size = vm["insert_delete_checkpoint_size"].as<uint64_t>();
    } else {
        insert_delete_checkpoint_size = 10000;
    }


    if (vm.count("update_checkpoint_size")) {
        update_checkpoint_size = vm["update_checkpoint_size"].as<uint64_t>();
    } else {
        update_checkpoint_size = 10000;
    }

    if (vm.count("update_repeat_times")) {
        update_repeat_times = vm["update_checkpoint_size"].as<int>();
    } else {
        update_checkpoint_size = 10000;
    }


    if (vm.count("insert_delete_num_threads")) {
        insert_delete_num_threads = vm["insert_delete_num_threads"].as<int>();
    } else {
        insert_delete_num_threads = 1;
    }


    if (vm.count("insert_batch_size")) {
        insert_batch_size = vm["insert_batch_size"].as<uint64_t>();
    } else {
        insert_batch_size = 1;
    }

    if (vm.count("update_num_threads")) {
        update_num_threads = vm["update_num_threads"].as<int>();
    } else {
        update_num_threads = 1;
    }

    if (vm.count("mb_repeat_times")) {
        repeat_times = vm["mb_repeat_times"].as<int>();
    } else {
        repeat_times = 0;
    }

    if (vm.count("mb_checkpoint_size")) {
        mb_checkpoint_size = vm["mb_checkpoint_size"].as<uint64_t>();
    } else {
        mb_checkpoint_size = 10000;
    }

    if (vm.count("microbenchmark_num_threads")) {
        microbenchmark_num_threads = vm["microbenchmark_num_threads"].as<std::vector<int>>();
    }

    if (vm.count("mb_ts_types")) {
        std::vector<std::string> ts_types;
        ts_types = vm["mb_ts_types"].as<std::vector<std::string>>();
        for (auto ts_type : ts_types) {
            mb_ts_types.push_back(get_ts_type(ts_type));
        }
    }

    if (vm.count("mb_operation_types")) {
        std::vector<std::string> op_types;
        op_types = vm["mb_operation_types"].as<std::vector<std::string>>();
        for (auto op_type : op_types) {
            mb_operation_types.push_back(get_op_type(op_type));
        }
    }

    if (vm.count("query_num_threads")) {
        query_num_threads = vm["query_num_threads"].as<std::vector<int>>();
    }

    if (vm.count("query_operation_types")) {
        std::vector<std::string> op_types;
        op_types = vm["query_operation_types"].as<std::vector<std::string>>();
        for (auto op_type : op_types) {
            query_operation_types.push_back(get_op_type(op_type));
        }
    }

    if (vm.count("alpha")) {
        alpha = vm["alpha"].as<int>();
    } else {
        alpha = 15;
    }

    if (vm.count("beta")) {
        beta = vm["beta"].as<int>();
    } else {
        beta = 18;
    }

    if (vm.count("bfs_source")) {
        bfs_source = vm["bfs_source"].as<uint64_t>();
    } else {
        bfs_source = 0;
    }

    if (vm.count("delta")) {
        delta = vm["delta"].as<double>();
    } else {
        delta = 2.0;
    }

    if (vm.count("sssp_source")) {
        sssp_source = vm["sssp_source"].as<uint64_t>();
    } else {
        sssp_source = 0;
    }

    if (vm.count("num_iterations")) {
        num_iterations = vm["num_iterations"].as<int>();
    } else {
        num_iterations = 10;
    }

    if (vm.count("damping_factor")) {
        damping_factor = vm["damping_factor"].as<double>();
    } else {
        damping_factor = 0.85;
    }

    if (vm.count("writer_threads")) {
        writer_threads = vm["writer_threads"].as<int>();
    } else {
        writer_threads = 16;
    }

    if (vm.count("reader_threads")) {
        reader_threads = vm["reader_threads"].as<int>();
    } else {
        reader_threads = 16;
    }

    if (vm.count("num_threads_search")) {
        num_threads_search = vm["num_threads_search"].as<int>();
    } else {
        num_threads_search = 8;
    }

    if (vm.count("num_threads_scan")) {
        num_threads_scan = vm["num_threads_scan"].as<int>();
    } else {
        num_threads_scan = 20;
    }

    if (vm.count("concurrent_workloads")) {
        auto workload_params = vm["concurrent_workloads"].as<std::vector<std::string>>();
        bool insert_found = false;
        
        for (const auto & params : workload_params) {
            concurrent_workload wl;
            std::istringstream iss(params);
            std::string token;

            while (std::getline(iss, token, ' ')) {
                auto pos = token.find(':');
                if (pos != std::string::npos) {
                    std::string key = token.substr(0, pos);
                    std::string value = token.substr(pos + 1);

                    if (key == "workload_type") {
                        wl.workload_type = get_op_type(value);
                        if (wl.workload_type == operationType::INSERT) {
                            if (insert_found) throw std::runtime_error("concurrent workload allows only one insert stream");
                            insert_found = true;
                        }
                    } else if (key == "target_stream_type") {
                        wl.target_stream_type = get_ts_type(value);
                    } else if (key == "num_threads") {
                        wl.num_threads = std::stoi(value);
                    }
                }
            }
            if (wl.workload_type == operationType::INSERT) concurrent_workloads.insert(concurrent_workloads.begin(), wl);
            else concurrent_workloads.push_back(wl);
        }
        if (!insert_found) {
            throw std::runtime_error("concurrent workload needs one insert stream");
        }
    }
    
    if (vm.count("num_threads")) {
        m_num_threads = vm["num_threads"].as<int>();
    } else {
        m_num_threads = 1;
    }

    if (vm.count("seed")) {
        m_seed = vm["seed"].as<int>();
    } else {
        m_seed = 0;
    }

    if (vm.count("workload_type")) {
        const std::set<std::string> workload_types = {"insert", "batch_insert", "delete", "update", "micro_benchmark", "bfs", "sssp", "pr", "cc", "tc", "tc_op", "query", "mixed", "concurrent", "qos"};
        std::string workload_type = vm["workload_type"].as<std::string>();
        if (workload_types.find(workload_type) == workload_types.end()) {
            std::cout << "Workload type is invalid.\n";
            exit(1);
        }

        m_workload_type = get_op_type(workload_type);
        
    } else {
        std::cout << "Workload type was not set.\n";
    }

    if (vm.count("target_stream_type")) {
        const std::set<std::string> target_stream_types = {"full", "general", "high_degree", "low_degree", "uniform", "based_on_degree"};
        std::string target_stream_type = vm["target_stream_type"].as<std::string>();
        if (target_stream_types.find(target_stream_type) == target_stream_types.end()) {
            std::cout << "Target stream type is not valid.\n";
            exit(1);
        }
        
        m_target_stream_type = get_ts_type(target_stream_type);

    } else {
        std::cout << "Target stream type was not set.\n";
    }

    if (vm.count("element_sizes")) {
        element_sizes = vm["element_sizes"].as<std::vector<int>>();
    }

    if (vm.count("real_graph")) {
        is_real_graph = vm["real_graph"].as<bool>();
    } else {
        is_real_graph = false;
    }

    if (vm.count("timestamp_rate")) {
        timestamp_rate = vm["timestamp_rate"].as<double>();
    } else {
        timestamp_rate = 0.0;
    }


    if (vm.count("num_vertices")) {
        m_num_vertices = vm["num_vertices"].as<uint64_t>();
    } else {
        m_num_vertices = 400;
    }

    if (vm.count("initial_graph_rate")) {
        initial_graph_rate = vm["initial_graph_rate"].as<double>();
    } else {
        initial_graph_rate = 0.8;
    }


    if (vm.count("num_search")) {
        num_search = vm["num_search"].as<uint64_t>();
    }


    if (vm.count("num_of_vertices")) {
        num_of_vertices = vm["num_of_vertices"].as<uint64_t>();
    } else {
        num_of_vertices = 1024;
    }

    
    if (vm.count("neighbor_size")) {
        neighbor_size = vm["neighbor_size"].as<uint64_t>();
    } else {
        neighbor_size = 1024;
    }


    if (vm.count("num_scan")) {
        num_scan = vm["num_scan"].as<uint64_t>();
    }

    if (vm.count("neighbor_test_repeat_times")) {
        neighbor_test_repeat_times = vm["neighbor_test_repeat_times"].as<int>();
    }

    if (vm.count("test_version_chain")) {
        test_version_chain = vm["test_version_chain"].as<bool>();
    }

    if (vm.count("version_chain_length")) {
        version_chain_length = vm["version_chain_length"].as<int>();
    }


    if (vm.count("version_rate")) {
        version_rate = vm["version_rate"].as<double>();
    }


    if (vm.count("is_shuffle")) {
        is_shuffle = vm["is_shuffle"].as<bool>();
    }


}

std::string commandLineParser::get_workload_dir() {
    return m_workload_dir;
}

std::string commandLineParser::get_output_dir() {
    return m_output_dir;
}

int commandLineParser::get_num_threads() {
    return m_num_threads;
}

int commandLineParser::get_seed() {
    return m_seed;
}

uint64_t commandLineParser::get_num_vertices() {
    return m_num_vertices;
}

 bool commandLineParser::get_real_graph() {
    return is_real_graph;
 }

 double commandLineParser::get_initial_graph_rate() {
    return initial_graph_rate;
 }

operationType commandLineParser::get_workload_type() {
    return m_workload_type;
}

targetStreamType commandLineParser::get_target_stream_type() {
    return m_target_stream_type;
}

Config commandLineParser::get_exp_config() {
    return Config(timestamp_rate);
}


uint64_t commandLineParser::get_insert_delete_checkpoint_size() {
    return insert_delete_checkpoint_size;
}

int commandLineParser::get_insert_delete_num_threads() {
    return insert_delete_num_threads;
}

int commandLineParser::get_update_num_threads() {
    return update_num_threads;
}

int commandLineParser::get_repeat_times() {
    return repeat_times;
}

uint64_t commandLineParser::get_mb_checkpoint_size() {
    return mb_checkpoint_size;
}

std::vector<int> commandLineParser::get_microbenchmark_num_threads() {
    return microbenchmark_num_threads;
}

int commandLineParser::get_alpha() {
    return alpha;
}

int commandLineParser::get_beta() {
    return beta;
}

uint64_t commandLineParser::get_bfs_source() {
    return bfs_source;
}

double commandLineParser::get_delta() {
    return delta;
}

uint64_t commandLineParser::get_sssp_source() {
    return sssp_source;
}

int commandLineParser::get_num_iterations() {
    return num_iterations;
}

double commandLineParser::get_damping_factor() {
    return damping_factor;
}

int commandLineParser::get_writer_threads() {
    return writer_threads;
}

int commandLineParser::get_reader_threads() {
    return reader_threads;
}

int commandLineParser::get_num_threads_search() {
    return num_threads_search;
}

int commandLineParser::get_num_threads_scan() {
    return num_threads_scan;
}

std::vector<int> commandLineParser::get_query_num_threads() {
    return query_num_threads;
}

std::vector<int>&& commandLineParser::get_element_sizes() {
    return std::move(element_sizes);
}


uint64_t commandLineParser::get_num_search() {
    return num_search;
}

uint64_t commandLineParser::get_num_scan() {
    return num_scan;
}

int commandLineParser::get_neighbor_test_repeat_times() {
    return neighbor_test_repeat_times;
}

bool commandLineParser::get_test_version_chain() {
    return test_version_chain;
}

double commandLineParser::get_timestamp_rate() {
    return timestamp_rate;
}

DriverConfig commandLineParser::get_driver_config() {
    DriverConfig config;
    config.workload_dir = m_workload_dir;
    config.output_dir = m_output_dir;
    config.workload_type = m_workload_type;
    config.target_stream_type = m_target_stream_type;

    // insert / delete config
    config.insert_delete_checkpoint_size = insert_delete_checkpoint_size;
    config.insert_delete_num_threads = insert_delete_num_threads;
    config.insert_batch_size = insert_batch_size;

    // update config
    config.update_checkpoint_size = update_checkpoint_size;
    config.update_num_threads = update_num_threads;
    config.update_repeat_times = update_repeat_times;

    // microbenchmark config
    config.repeat_times = repeat_times;
    config.mb_checkpoint_size = mb_checkpoint_size;
    config.microbenchmark_num_threads = microbenchmark_num_threads;
    config.mb_operation_types = mb_operation_types;
    config.mb_ts_types = mb_ts_types;

    //query 
    config.query_num_threads = query_num_threads;
    config.query_operation_types = query_operation_types;
    
    // bfs
    config.alpha = alpha;
    config.beta = beta;
    config.bfs_source = bfs_source;

    // sssp
    config.delta = delta;
    config.sssp_source = sssp_source;

    // pr
    config.num_iterations = num_iterations;
    config.damping_factor = damping_factor;

    // mixed
    config.writer_threads = writer_threads;
    config.reader_threads = reader_threads;

    // qos
    config.num_threads_search = num_threads_search;
    config.num_threads_scan = num_threads_scan;

    // concurrent
    config.concurrent_workloads = concurrent_workloads;

    return config;
}


EdgeDriverConfig commandLineParser::get_edge_driver_config() {
    EdgeDriverConfig config;

    config.workload_dir = m_workload_dir;
    config.output_dir = m_output_dir;
    config.workload_type = m_workload_type;
    config.target_stream_type = m_target_stream_type;

    config.mb_operation_types = mb_operation_types;
    config.mb_ts_types = mb_ts_types;

    config.element_sizes = element_sizes;
    config.neighbor_size = neighbor_size;
    config.num_of_vertices = num_of_vertices;
    config.timestamp_rate = timestamp_rate;
    config.initial_graph_rate = initial_graph_rate;
    config.num_search = num_search;
    config.num_scan = num_scan;
    config.repeat_times = neighbor_test_repeat_times;
    config.test_version_chain = test_version_chain;
    config.version_chain_length = version_chain_length;
    config.is_real_graph = is_real_graph;
    config.is_shuffle = is_shuffle;
    config.version_rate = version_rate;

    return config;
}
