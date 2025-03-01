#pragma once
#include <iostream>
#include "utils/commandLineParser.hpp"
#include "types/types.hpp"
#include <unordered_set>

#include "vertex_driver.h"
// #include "ittnotify.h"

int main(int argc, char** argv) {
    // __itt_pause();
    commandLineParser& parser = commandLineParser::get_instance();
    parser.parse("/path/to/vertex/wrapper/dir/config.cfg");
    
    auto tp = parser.get_workload_type();
    bool is_insert_task = tp == operationType::INSERT;
    execute(parser.get_num_vertices(), parser.get_output_dir(), is_insert_task, parser.get_initial_graph_rate(), parser.get_seed());

    return 0;
}
