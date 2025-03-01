#include <iostream>
#include "utils/commandLineParser.hpp"
#include "types/types.hpp"
#include <unordered_set>
#include <linux/perf_event.h>
#include <unistd.h>

#include "edge_driver.h"
#include "utils/log/log.h"

int main(int argc, char** argv) {
    commandLineParser& parser = commandLineParser::get_instance();
    parser.parse("/path/to/your/config.cfg");
    
    if (parser.get_real_graph()) {
        std::string vertex_path = parser.get_workload_dir() + "/initial_stream_insert_vertex.stream";
        std::vector<operation> vertex_stream;
        read_stream(vertex_path, vertex_stream);
        
        execute_real_graph(vertex_stream.size(), parser.get_edge_driver_config());
    }
    else {
        execute(parser.get_edge_driver_config());
    }

    return 0;
}

