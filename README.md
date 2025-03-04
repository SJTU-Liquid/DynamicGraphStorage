# Contants

1. [How to Build](#1-how-to-build)
2. [Generate Workloads](#2-generate-workloads)
3. [Process Datasets](#3-process-datasets)
4. [Composable Dynamic Graph Storage (DGS)](#4-composable-dynamic-graph-storage-dgs)
5. [Test Driver](#5-test-driver)

# 1. Build

## Prerequisites

Ensure your system meets the following requirements:

- **C++ Standard**: The project requires **C++20**. Please ensure your compiler supports C++20 features.

- **Compiler**: GCC version **10.0** or higher.

- [Boost Library](https://www.boost.org/): This project requires **Boost version 1.74 or higher**.

  ```
  sudo apt install libboost-all-dev
  ```

- [Intel TBB](https://www.intel.com/content/www/us/en/developer/tools/oneapi/onetbb-download.html): This project requires **Intel Threading Building Blocks (TBB)** version **2021.1 or higher**.

  ```
  sudo apt install libtbb-dev
  ```

## Configuration

Modify configuration file path defined in [./driver/driver_main.h](./driver/driver_main.h) and [./edge_wrapper/edge_driver_main.h](./edge_wrapper/edge_driver_main.h) to be your own.

```cpp
parser.parse("/path/to/your/dir/config.cfg");
```

## Build

please replace `/path/to/your/dir` in `container/utils/PAM/_deps/parlaylib-subbuild/CMakeLists.txt` as your own path.

```bash
git clone https://github.com/SJTU-Liquid/DynamicGraphStorage.git
mkdir build && cd build
cmake ..
make
```

# 2. Generate workloads

## Workloads

[./generator](./generator) generates workloads for test use. Workloads include initial stream and target stream. The initial stream includes insert operations that load the initial graph for target stream tests. The target stream includes operations that are executed by the driver.

### Operation Types

#### Insert Workloads

* **Initial Stream**
  * `initial_initial_stream_insert_${target_stream_type}.stream`: Contains the initial graph to be loaded.
* **Target Stream**
  * `target_initial_stream_insert_${target_stream_type}.stream`: Contains the rest of the graph to be tested.

#### Micro Benchmark Tests

* **Initial Stream**
  * `target_initial_stream_insert_full.stream`: Contains the whole graph to be initialized.
* **Target Stream**
  * `target_initial_stream_scan_neighbor_${target_stream_type}.stream`: Contains the `scan` workload for micro benchmark tests.
  * `target_initial_stream_get_edge_${target_stream_type}.stream`: Contains the `search` workload for micro benchmark tests.

### Target Stream Types

* **full**
  * Used only for inserting the entire graph.
* **general**
  * Randomly selects a certain ratio of edges (for `insert` and `search`) and vertices (for `scan`). The ratio for `insert`, `search`, and `scan` is controlled by the parameters `--initial_graph_ratio`, `edge_query_ratio`, and `vertex_query_ratio`, respectively.
* **based_on_degree**
  * Selects edges and vertices based on degree. This means a certain vertex `v` or an edge with its source being `v` is chosen with a probability of $\frac{deg(v)}{\sum_i(deg(i))}$.
* **uniform**
  * Selects edges and vertices uniformly. This means that a certain vertex `v` or an edge with its source being `v` is chosen with a probability of $\frac{1}{|V|}$.

## Config

Detailed instructions on how to use the `commandLineParser` to parse command line arguments. Following could be defined in `config.cfg`

- `input_file` : Specifies the path to the input file. Example: `input_file=path/to/file`
- `output_dir`: Specifies the directory to store the output workloads. Example: `output_dir=path/to/file`
- `is_weighted` : Indicates whether the graph is weighted. Use `true` or `false`. Default is `false`. Example: `is_weighted=true`
- `is_shuffle`: Indicates whether the insertion order is shuffled. Use `true` or `false`. Default is `false`. Example: `is_shuffle=true`
- `delimiter` : Specifies the delimiter used in the input file. Options include `tab`, `space` and `comma`. Default is space. Example: `delimiter=comma`
- `initial_graph_ratio`: Specifies the initial graph ratio to the total graph in general workloads. Default is `0.8`. Example: `initial_graph_ratio=0.5`
- `vertex_query_ratio`: Specifies the vertex query ratio of general workloads in micro-benchmarks. Default is `0.2`. Example: `vertex_query_ratio=0.4`
- `edge_query_ratio`: Specifies the edge query ratio of general workloads in micro-benchmarks. Default is `0.2`. Example: `edge_query_ratio=0.4`
- `seed` : Sets the random seed. Default is `0`. Example: `seed=19260817`
- `uniform_based_on_degree_size`: Specifies the operation number of uniform based on degree workloads. Default is `3200000`. Example: `uniform_based_on_degree_size=3200000`

## Input File Format

The input should be a file where each line contains `source, destination` for unweighted graphs or `source, destination, weight` for weighted graphs. Unweighted graphs will be randomly assigned a double weight.

## Generator Usage

Use [Patent citation network](https://snap.stanford.edu/data/cit-Patents.html) as example to generate workloads.

### Download dataset

```bash
 mkdir cit && cd cit
 wget https://snap.stanford.edu/data/cit-Patents.txt.gz
 gzip -d cit-Patents.txt.gz
 mkdir workloads
```

### Generate workloads

Example config.cfg file: 

```
# Configuration file for the application

# General options
input_file = /path/to/your/dir/cit-Patents.txt
output_dir = /path/to/your/dir/workloads

# Graph properties
is_weighted = false
is_shuffle = true
delimiter = tab

# Ratios
initial_graph_ratio = 0.8
vertex_query_ratio = 0.2
edge_query_ratio = 0.2
high_degree_vertex_ratio = 0.01
high_degree_edge_ratio = 0.2
low_degree_vertex_ratio = 0.2
low_degree_edge_ratio = 0.5
uniform_based_on_degree_size = 320000

# Random seed
seed = 0
```

Run command:

```bash
 ./build/generator/workloads_generator /path/to/your/cfg/dir/config.cfg
```

# 3. Process datasets

We provide generated workloads of the following datasets at [Zenodo](https://zenodo.org/).

[Workload Download 1](https://zenodo.org/records/14609233?token=eyJhbGciOiJIUzUxMiJ9.eyJpZCI6IjIwMTA0MzNkLWQ4ZmUtNGVmNS1iMGIxLWU5MGNiNDUwYTQ3ZiIsImRhdGEiOnt9LCJyYW5kb20iOiIwMDIyYTllYWE3ZjZhZDdkYWE5NDQwOTNiY2NkNWY4ZiJ9.KgZd_Kyo2tZZFZWQJzkr9gYYH7J11XcNi9Qp1Um7lJdaDk_hKctENPNtJaXPGzvr6onOKP08IgVeRqk3GnY62w)

[Workload Download 2](https://zenodo.org/records/14920946?token=eyJhbGciOiJIUzUxMiJ9.eyJpZCI6ImEyMzZjZjZkLTQ4MzQtNDEwMi05OTIwLTZjZDg5MTlmYzkyNiIsImRhdGEiOnt9LCJyYW5kb20iOiI2ZTA0YjNmOGQyN2ViMmVlMDBmMTQxNjM3NzUwOWFkNiJ9.00ktBCkrySWYE5mSU5RNmtk1FHKbprRj_rXtzOWZYFN14EJRRZn-LS87Gk_m8rTbb_BI-3oYcZj3eZz6UpMfLA)

* liveJournal
  * Downloaded from [SNAP: Network datasets: LiveJournal social network (stanford.edu)](https://snap.stanford.edu/data/soc-LiveJournal1.html).
* dotaLeague
  * Downloaded from [LDBC Graphalytics Benchmark (LDBC Graphalytics) (ldbcouncil.org)](https://ldbcouncil.org/benchmarks/graphalytics/)
* LDBC
  * Generated from [ldbc/ldbc_snb_datagen_hadoop: The Hadoop-based variant of the SNB Datagen (github.com)](https://github.com/ldbc/ldbc_snb_datagen_hadoop/).
  * Parameters
    * scale facter: sf10
    * serializer: CsvBasic
    * dataformatter: LongDateFormatter
  * Processed and uniformed by [./data_process/ldbc](./data_process/ldbc)
* Graph500
  * Downloaded from  [LDBC Graphalytics Benchmark (LDBC Graphalytics) (ldbcouncil.org)](https://ldbcouncil.org/benchmarks/graphalytics/)
  * Parameters
    * scale factor: 24
* Wiki
  * Downloaded from [SNAP: Network datasets: Wikipedia edit history (stanford.edu)](https://snap.stanford.edu/data/wiki-meta.html)
  * Wiki is a bipartite graph where the IDs of editors and articles provided in the dataset have duplicates and include anonymous editor IDs. Therefore, preprocessing is required to handle this issue. Origin file is processed by [./data_process/wiki.py](./data_process/wiki.py)
* Cit
  * Downloaded from [SNAP: Network datasets: US Patent citation network (stanford.edu)](https://snap.stanford.edu/data/cit-Patents.html)
* Twitter
  * Downloaded from  https://nrvis.com/download/data/soc/soc-twitter-2010.zip                 
* NFT
  * [Live Graph Lab: Towards Open, Dynamic and Real Transaction Graphs with NFT](https://livegraphlab.github.io/#about)
  * Selected the first 200,000,000 transaction records, used from and to ID pairs as edges and removed duplicate edges and self-loops.

# 4. Composable Dynamic Graph Storage (DGS)

We implemented the composable dynamic graph storage system in `./container`. 

## Container

### Vertex Index

Vertex index data structures  are defined in `./container/vertex_index`.

* vector  [./container/vertex_index/vector.hpp](./container/vertex_index/vector.hpp)
* hashmap   [./container/vertex_index/hashmap.hpp](./container/vertex_index/hashmap.hpp)
* PAM tree  [./container/vertex_index/avltree_cow.hpp](./container/vertex_index/avltree_cow.hpp)

### Edge Index

Edge index data structures are defined in `./container/edge_index`

* unrolled skip list  [./container/edge_index/skiplist.hpp](./container/edge_index/skiplist.hpp)
* Log Block   [./container/edge_index/logblock.hpp](./container/edge_index/logblock.hpp)
* unrolled PAM   [./container/edge_index/pam_tree_cow.hpp](./container/edge_index/pam_tree_cow.hpp)
* sorted array  [./container/edge_index/sorted_array.hpp](./container/edge_index/sorted_array.hpp)
* PMA  [./container/edge_index/pma.hpp](./container/edge_index/pma.hpp)

### Container and Transaction

`./container/container` defines how to compose [vertex_index](#Vertex Index) and [edge_index](#Edge Index) together.

`./container/transaction` defines the concurrency control protocol used.

* **2PL** [./container/container/container_2pl.hpp](./container/container/container_2pl.hpp) + [./container/transaction/transaction_2pl.hpp](./container/transaction/transaction_2pl.hpp): Compose [vertex_index](#Vertex Index) and [edge_index](#Edge Index) together and use 2PL concurrency control protocol, corresponding to the fine-grained concurrency control in the paper.
* **Copy-on-write**: [./container/container/container_pam.hpp](./container/container/container_pam.hpp) + [./container/transaction/transaction_pam.hpp](./container/transaction/transaction_pam.hpp): Compose [vertex_index](#Vertex Index) and [edge_index](#Edge Index) together and use copy-on-write strategy, corresponding to the coarse-grained concurrency control in the paper. Currently support [./container/vertex_index/avltree_cow.hpp](./container/vertex_index/avltree_cow.hpp) +  [./container/edge_index/pam_tree_cow.hpp](./container/edge_index/pam_tree_cow.hpp).

### Compose as DGS

Composed DGSs are defined in `./container/apps`. Follow the definitions in [./container/wrapper.h](./container/wrapper.h). Follow the example of [./container/apps/vector2sorted_array.cpp](./container/apps/vector2sorted_array.cpp) to compose new DGSs.

### Container configuration

The following preprocessor definitions are used to configure the workload generator and its associated operations:

* `-DENABLE_TIMESTAMP`: Enables the inclusion of timestamps vertex and edge records.
* `-DENABLE_LOCK`: Enables locking mechanisms during workload operations for thread safety.
* `-DBLOCK_SIZE_VALUE`: Sets the block size value. This parameter determines the size of blocks used in certain operations.
  * Used for neighbor index `unrolled PAM`,  `PMA` and `unrolled skip list`. 
  * `BLOCK_SIZE_VALUE` = element per block.
* `-DENABLE_ADAPTIVE`: Enable adaptive indexing of neighbor index. 
* `-DDEFAULT_VECTOR_SIZE_VALUE`: Sets the size (number of elements) for adaptive indexing of neighbor index `unrolled skip list`.
* `-DENABLE_GC`: Enables garbage collection for 2PL DGSs.
* `-DENABLE_FLAT_SNAPSHOT`: Enable flattened snapshot for [./container/vertex_index/avltree_cow.hpp](./container/vertex_index/avltree_cow.hpp) +  [./container/edge_index/pam_tree_cow.hpp](./container/edge_index/pam_tree_cow.hpp). If enabled, the AVL tree vertex index will be flattened to vector when creating read-only snapshots to accelerate vertex indexing.

## Graph container test driver

### Vertex test

[./container/vertex_wrapper](./container/vertex_wrapper)

#### Configuration

Modify [./container/vertex_wrapper/config.cfg](./container/vertex_wrapper/config.cfg).

The following configuration parameters are used to set up and execute the workload generator:

* `output_dir = /path/to/output/dir`: Specifies the directory where the test results will be stored.
* `workload_type`: Defines the type of workload to be executed, in this case, an insert workload. `insert` and `micro_benchmark` are supported.
* `initial_graph_rate = 0.8`: Used for `insert` workload. Sets the initial graph rate to 0.8, indicating that 80% of the graph will be pre-loaded, and the rest is for insert tests.
* `num_vertices`: Specifies the number of vertices in the graph.

These parameters can be adjusted according to your specific requirements to tailor the workload generator's behavior and output.

#### Run

```
./build/container/vector_vertex_test
./build/container/hashmap_vertex_test
./build/container/art_vertex_test
```

### Neighbor index test

[./edge_wrapper](./edge_wrapper)

#### Configuration

Modify [./edge_wrapper/config.cfg](./edge_wrapper/config.cfg).

The following configuration parameters are used to set up and execute the workload generator:

##### General Settings

* `output_dir = /path/to/your/output/dir`: Specifies the directory where the test results will be stored.
* `workload_type`: Defines the type of workload to be executed. Supports `micro_benchmark` and `insert`.

##### Real Graph Settings

* `real_graph = true`: Indicates that the workload will be executed on a real graph defined by `workload_dir`
* `workload_dir = /path/to/your/output/dir`: Specifies the directory containing the workloads for the real graph.
* `target_stream_type`: Specifies the target stream type for `insert`. Supports `full`, `general` and `based_on_degree` (depending on workload generated).
* `ms_ts_types`: Specifies the target stream type for `micro_benchmark`. Could be re-defined multiple times as they operate on the same initial graph snapshot. Supports `general` and `based_on_degree` (depending on workload generated).

##### Synthetic Graph Settings

* `element_sizes`: Sets the neighbor size of each vertex for the synthetic graph. `20` meas each vertex has the neighborhood size of $2^{20}$
* `initial_graph_rate = 0.8`: Sets the initial graph rate to 0.8, indicating that 80% of the graph will be pre-loaded.
* `initial_timestamp = 0.8`: Sets the initial timestamp rate to 0.8, which means if timestamp is enabled, scan and search operations will be executed with the timestamp of `0.8 * global timestamp`
* `test_version_chain = true`: Enables testing of the version chain.
* `timestamp_rate = 0.00`: Specifies the ratio of edges that has version chains in the synthetic graph.
* `num_search = 100000`: Specifies the number of search operations to be performed for `micro_benchmark`.

#### Sample Configuration

```
workload_dir = /path/to/your/workloads/dir
output_dir = /path/to/your/workload/dir
workload_type = insert
target_stream_type = general

mb_operation_types = get_edge
mb_operation_types = scan_neighbor

mb_ts_types = based_on_degree
mb_ts_types = general

real_graph = true
timestamp_rate = 0.00
initial_graph_rate = 0.8
num_of_vertices = 1048576
neighbor_size = 16
num_search = 100000
num_scan = 1048576
is_shuffle = true
version_rate = 0.8
neighbor_test_repeat_times = 1
```

#### Run

This project includes pre-defined **Neighbor Index Tests** with various block sizes and , ranging from $2^4$ to $2^{10}$. Each test is tailored to evaluate performance under different configurations of block sizes. The suffix of each executable represents the block size. For example, 4 represents a block size of $2^4 = 16$. Both **versioned** and **unversioned** executables are pre-defined:

For example:

```bash
./build/edge_wrapper/pma_edge_test_4
./build/edge_wrapper/pam_edge_test_5
./build/edge_wrapper/skiplist_edge_test_6
./build/edge_wrapper/logblock_edge_test_7
./build/edge_wrapper/sorted_array_edge_test_8

./build/edge_wrapper/pma_edge_test_unversioned_9
./build/edge_wrapper/pam_edge_test_unversioned_10
./build/edge_wrapper/skiplist_edge_unversioned_4
./build/edge_wrapper/logblock_edge_unversioned_5
./build/edge_wrapper/sorted_array_unversioned_6
```

# 5. Test Driver

`Dynamic Graph Storage Sandbox` are implemented in [./container/apps](./container/apps). Test driver is implemented in [./driver/driver.h](./driver/driver.h).

## Configuration

The following configuration parameters are used to set up and execute the workload generator. Adjust these parameters according to your specific requirements.

### General Settings

* `workload_dir`: Specifies the directory containing the workloads that are generated by the workload generator.
  * Example: `/path/to/your/workloads`
* `output_dir`: Specifies the directory where the test results will be stored.
  * Example: `/path/to/your/output/directory`
* `workload_type`: Defines the type of workload to be executed.
  * Supports: `insert`, `micro_benchmark`,`mixed`,  `query`, `qos`, `concurrent`

### Insert Configuration

Set `workload_type` to be `insert` or `batch_insert`.

* `insert_delete_checkpoint_size`: Sets the checkpoint size for insert operations.
  * Example: `1000000`
* `insert_delete_num_threads`: Specifies the number of threads for insert operations.
  * Example: `1`
* `insert_batch_size`: Specifies the batch_size for insert operations when the `workload_type` is set to `batch_insert`.
  * Example: 1024
* `target_stream_type`: Indicates the method for selecting edges and vertices.
  * Supports: `full`, `general`, `based_on_degree`

### Update Configuration

Set `workload_type` to be `update`.

* `update_checkpoint_size`: Sets the checkpoint size for update operations.
  * Example: `10000`
* `update_num_threads`: Specifies the number of threads for update operations.
  * Example: `10`

### Microbenchmark Configuration

Set `workload_type` to be `micro_benchmark`.

* `mb_repeat_times`: Specifies the number of times the microbenchmark should be repeated.
  * Example: `0`
* `mb_checkpoint_size`: Sets the checkpoint size for microbenchmark operations.
  * Example: `100`
* `microbenchmark_num_threads`: Specifies the number of threads for microbenchmark operations.
  * Examples: `1`, `2`, `4`, `8`, `16`, `32`
* `mb_operation_types`: Defines the types of operations for the microbenchmark.
  * Examples: `get_edge`, `scan_neighbor`
* `mb_ts_types`: Defines the types of timestamp selection for the microbenchmark.
  * Supports: `based_on_degree`, `general`

### Query Configuration

Set `workload_type` to be `query`.

* `query_num_threads`: Specifies the number of threads for query operations.
  * Examples: `1`, `2`
* `query_operation_types`: Defines the types of query operations.
  * Supports: `bfs`, `sssp`, `pr`, `wcc`, `tc`, `tc_op`
  * `bfs`, `sssp`, `pr`, `wcc` query follow the implementation of [GAP Benchmark Suite](https://github.com/sbeamer/gapbs.git), with parameters set according to its implementation.

#### BFS (Breadth-First Search) Configuration

* `alpha`: Parameter for BFS.
  * Example: `15`
* `beta`: Parameter for BFS.
  * Example: `18`
* `bfs_source`: Specifies the source vertex for BFS.
  * Example: `0`

#### SSSP (Single Source Shortest Path) Configuration

* `delta`: Parameter for SSSP.
  * Example: `2.0`
* `sssp_source`: Specifies the source vertex for SSSP.
  * Example: `0`

#### PageRank (PR) Configuration

* `num_iterations`: Specifies the number of iterations for the PageRank algorithm.
  * Example: `10`
* `damping_factor`: Sets the damping factor for the PageRank algorithm.
  * Example: `0.85`

### Mixed Configuration

Set `workload_type` to be `mixed`.

* `writer_threads`: Specifies the number of writer threads.
  * Example: `0`
* `reader_threads`: Specifies the number of reader threads.
  * Example: `32`

### QoS (Quality of Service) Configuration

Set `workload_type` to be `qos`.

* `num_threads_search`: Specifies the number of threads for search operations.
  * Example: `8`
* `num_threads_scan`: Specifies the number of threads for scan operations.
  * Example: `20`

### Concurrent Configuration

Set `workload_type` to be `concurrent`.

* `Workload Type`: Specifies the type of operation to be performed (e.g., scan_neighbor, insert, get_edge).
* `Target Stream Type`: Determines the stream behavior, such as based_on_degree or general.
* `Number of Threads`: Defines the number of threads allocated for each workload.

**Specify a kind of workload by:** 

```
concurrent_workloads = workload_type:<workload_type> target_stream_type:<target_stream_type> num_threads:<number_of_threads>
```

**Example Usage:**

```
concurrent_workloads = workload_type:scan_neighbor target_stream_type:based_on_degree num_threads:4
concurrent_workloads = workload_type:insert target_stream_type:general num_threads:1
concurrent_workloads = workload_type:get_edge target_stream_type:based_on_degree num_threads:1
```

The example above initializes graph using `intial_stream_insert_general.stream`. Then execute four scan threads of based_on_degree workloads, one insert thread using general workload and one search thread using based_on_degree workload.

### Example

```
workload_dir = /path/to/your/workload/dir
output_dir = /path/to/your/output/dir
workload_type = insert
target_stream_type = full

# insert / delete config
insert_delete_checkpoint_size = 1000000
insert_delete_num_threads = 1

# update config
update_checkpoint_size = 10000
update_num_threads = 10

# microbenchmark config
mb_repeat_times = 0
mb_checkpoint_size = 100
microbenchmark_num_threads = 1
microbenchmark_num_threads = 2
 
mb_operation_types = get_edge
mb_operation_types = scan_neighbor
mb_ts_types = based_on_degree
mb_ts_types = general



# query
query_num_threads = 1
query_num_threads = 2
query_operation_types = bfs
query_operation_types = sssp
query_operation_types = pr
query_operation_types = wcc
query_operation_types = tc
query_operation_types = tc_op

# bfs
alpha = 15
beta = 18
bfs_source = 0

# sssp
delta = 2.0
sssp_source = 0

# pr
num_iterations = 10
damping_factor = 0.85

# mixed
writer_threads = 0
reader_threads = 32

# qos
num_threads_search = 8
num_threads_scan = 20


#concurrent
concurrent_workloads = workload_type:scan_neighbor target_stream_type:based_on_degree num_threads:4
concurrent_workloads = workload_type:insert target_stream_type:general num_threads:1
concurrent_workloads = workload_type:get_edge target_stream_type:based_on_degree num_threads:1
```

## Run

```
# Predefined Dynamic Graph Storage Sandbox
./build/container/vector2pma
./build/container/vector2skiplist
./build/container/vector2skiplist_adaptive
./build/container/vector2logblock
./build/container/vector2sorted_array
./build/container/avltree2pam_cow
./build/container/avltree2pam_cow_p
./build/container/avltree2pam_cow_flat

./build/container/vector2pma_unversioned
./build/container/vector2skiplist_unversioned
./build/container/vector2logblock_unversioned
./build/container/vector2sorted_array_unversioned
```

## Third-party Modules

Download the official libraries to the [./libraries](./libraries) directory and integrate them using the APIs defined in [./wrapper/apps](./wrapper/apps). Users can add the corresponding subdirectory in [./CMakeLists.txt](./CMakeLists.txt), and integrate them in [./wrapper/CMakeLists.txt](./wrapper/CMakeLists.txt) to create instances for evaluation.

* Download the official library of `LiveGraph` from [LiveGraph](https://github.com/thu-pacman/LiveGraph-Binary/releases). 
* Download the official library of `Teseo` from [Teseo](https://github.com/cwida/teseo).
  * Integrated using [./libraries/teseo/CMakeLists.txt](./libraries/teseo/CMakeLists.txt)
* Download official library of `aspen` from [Aspen](https://github.com/ldhulipala/aspen/). 
  * Integrated using [./libraries/teseo/CMakeLists.txt](./libraries/teseo/CMakeLists.txt)
* Download official library of `Sortledton` from [Sortledton](https://gitlab.db.in.tum.de/per.fuchs/sortledton)

