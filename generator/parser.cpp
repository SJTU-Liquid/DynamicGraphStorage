#include "parser.hpp"

namespace po = boost::program_options;

Parser& Parser::get_instance() {
    static Parser instance;
    return instance;
}

void Parser::parse(const std::string &config_file) {
    std::ifstream config(config_file);
    if (!config.is_open()) {
        std::cerr << "Error: cannot open config file" << std::endl;
        exit(1);
    }
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help,h", "produce help message")
        ("input_file, i", po::value<std::string>(), "input file")
        ("output_dir, o", po::value<std::string>(), "output directory")
        ("is_weighted, w", po::value<bool>()->default_value(false), "is the graph weighted")
        ("is_shuffle, w", po::value<bool>()->default_value(true), "is shuffle")
        ("delimiter,d", po::value<std::string>()->default_value("space"), "delimiter character (space/comma/tab)")
        ("initial_graph_ratio", po::value<double>()->default_value(0.8), "initial graph ratio")
        ("vertex_query_ratio", po::value<double>()->default_value(0.2), "vertex query ratio")
        ("edge_query_ratio", po::value<double>()->default_value(0.2), "edge query ratio")
        ("high_degree_vertex_ratio", po::value<double>()->default_value(0.01), "high degree vertex ratio")
        ("high_degree_edge_ratio", po::value<double>()->default_value(0.2), "high degree edge ratio")
        ("low_degree_vertex_ratio", po::value<double>()->default_value(0.2), "low degree vertex ratio")
        ("low_degree_edge_ratio", po::value<double>()->default_value(0.5), "low degree edge ratio")
        ("uniform_based_on_degree_size", po::value<uint64_t>()->default_value(320000), "uniform based on degree size")
        ("seed, s", po::value<unsigned int>()->default_value(0), "random seed")
    ;

    po::variables_map vm;
    po::store(po::parse_config_file(config, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        std::cout << desc << "\n";
        exit(0);
    }

    if (vm.count("input_file")) {
        input_file = vm["input_file"].as<std::string>();
    } else {
        std::cout << "Input edge file was not set.\n";
    }

    if (vm.count("output_dir")) {
        output_dir = vm["output_dir"].as<std::string>();
    } else {
        std::cout << "Output directory was not set.\n";
        exit(1);
    }

    if (vm.count("delimiter")) {
        std::string delimiter_value = vm["delimiter"].as<std::string>();
        if (delimiter_value == "space") {
            delimiter = ' ';
        } else if (delimiter_value == "comma") {
            delimiter = ',';
        } else if (delimiter_value == "tab") {
            delimiter = '\t';
        } else {
            throw std::invalid_argument("Invalid delimiter value. Use 'space', 'comma', or 'tab'.");
        }
    }

    is_weighted = vm["is_weighted"].as<bool>();
    is_shuffle = vm["is_shuffle"].as<bool>();
    
    initial_graph_ratio = vm["initial_graph_ratio"].as<double>();
    vertex_query_ratio = vm["vertex_query_ratio"].as<double>();
    edge_query_ratio = vm["edge_query_ratio"].as<double>();
    high_degree_vertex_ratio = vm["high_degree_vertex_ratio"].as<double>();
    high_degree_edge_ratio = vm["high_degree_edge_ratio"].as<double>();
    low_degree_vertex_ratio = vm["low_degree_vertex_ratio"].as<double>();
    low_degree_edge_ratio = vm["low_degree_edge_ratio"].as<double>();
    uniform_based_on_degree_size = vm["uniform_based_on_degree_size"].as<uint64_t>();
    seed = vm["seed"].as<unsigned int>();
}

std::string Parser::get_input_file() const {
    return input_file;
}


std::string Parser::get_output_dir() const {
    return output_dir;
}

bool Parser::get_is_weighted() const {
    return is_weighted;
}


bool Parser::get_is_shuffle() const {
    return is_shuffle;
}

char Parser::get_delimiter() const {
    return delimiter;
}

double Parser::get_initial_graph_ratio() const {
    return initial_graph_ratio;
}

double Parser::get_vertex_query_ratio() const {
    return vertex_query_ratio;
}

double Parser::get_edge_query_ratio() const {
    return edge_query_ratio;
}

double Parser::get_high_degree_vertex_ratio() const {
    return high_degree_vertex_ratio;
}

double Parser::get_high_degree_edge_ratio() const {
    return high_degree_edge_ratio;
}

double Parser::get_low_degree_vertex_ratio() const {
    return low_degree_vertex_ratio;
}

double Parser::get_low_degree_edge_ratio() const {
    return low_degree_edge_ratio;
}

unsigned int Parser::get_seed() const {
    return seed;
}

uint64_t Parser::get_uniform_based_on_degree_size() const {
    return uniform_based_on_degree_size;
}