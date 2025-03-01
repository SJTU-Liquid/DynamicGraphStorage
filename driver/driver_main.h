#pragma once
#include <iostream>
#include "utils/commandLineParser.hpp"

#include "driver.h"

int main(int argc, char** argv) {
  commandLineParser& parser = commandLineParser::get_instance();
  parser.parse("/path/to/your/config.cfg");

  // wrapper::execute(parser.get_workload_dir(), parser.get_output_dir(), parser.get_num_threads(), parser.get_workload_type(), parser.get_target_stream_type());
  wrapper::execute(parser.get_driver_config());
  return 0;
}
