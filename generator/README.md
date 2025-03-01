# Workload Generator

//TODO describe the workloads

### Prerequisites

This project requires the following environment and dependencies:

- C++ Compiler (supporting C++11 or later)
- [Boost Library](https://www.boost.org/) (specifically the Boost.Program_options module)

Make sure you have the Boost library installed on your system. Refer to the [Boost official documentation](https://www.boost.org/doc/libs) for installation instructions.

### Build

```bash
mkdir build && cd build
cmake
make
```

## Usage

Detailed instructions on how to use the `commandLineParser` to parse command line arguments:

- `--input_file` (or `-i`): Specifies the path to the input file. Example: `--input_file=path/to/file`
- `--output_dir` (or `-o`): Specifies the directory to store the output workloads. Example: `--output_dir=path/to/file`
- `--is_weighted` (or `-w`): Indicates whether the graph is weighted. Use `true` or `false`. Default is `false`. Example: `--is_weighted=true`
- `--delimiter` (or `-d`): Specifies the delimiter used in the input file. Default is a space. Example: `--delimiter=,`
- `--initial_graph_ratio`: Specifies the initial graph ratio to the total graph in general workloads. Default is `0.8`. Example: `--initial_graph_ratio=0.5`
- `--vertex_query_ratio` : Specifies the vertex query ratio in micro-benchmarks. Default is `0.2`. Example: `--vertex_query_ratio=0.3`
- `--edge_query_ratio`: Specifies the edge query ratio in micro-benchmarks. Default is `0.2`. Example: `--edge_query_ratio=0.4`
- `--high_degree_vertex_ratio`: The ratio of high-degree vertices in high degree workloads. Default is `0.01`. Example: `--high_degree_vertex_ratio=0.02`
- `--high_degree_edge_ratio`: The ratio of edges of high-degree vertices. Default is `0.2`. Example: `--high_degree_edge_ratio=0.3`
- `--repeat_times`: Specifies the number of repeat times in update workload. Default is `10`. Example: `--repeat_times=5`
- `--seed` (or `-s`): Sets the random seed. Default is `0`. Example: `--seed=12345`

## Input File Format

​	The input should be a CSV file where each line contains `source, destination` for unweighted graphs or `source, destination, weight` for weighted graphs. Unweighted graphs will be randomly assigned a double weight.

