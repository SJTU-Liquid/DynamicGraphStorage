cmake_minimum_required(VERSION 3.10)

project(teseo)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(${PROJECT_SOURCE_DIR}/include)


set(conf_async_num_threads 8)
set(conf_aux_counting_tree_capacity_inodes 512)
set(conf_aux_counting_tree_capacity_leaves 512)
set(conf_tclist_initial_capacity 1024)
set(conf_crawler_calibrator_tree_height 10)
set(conf_memstore_max_num_segments_per_leaf 256)
set(conf_memstore_payload_file_first_block_size 128)
set(conf_memstore_payload_file_next_block_size 128)
set(conf_memstore_segment_size 512)
set(conf_numa_enabled false)
set(conf_numa_num_nodes 1)
set(conf_runtime_num_threads 32)
set(conf_runtime_txnlist_refresh 500)
set(test_mode false)
set(conf_transaction_memory_pool_size 4096)
set(conf_vertex_table_min_capacity 1024)

configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/include/teseo/context/static_configuration.hpp.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/teseo/context/static_configuration.hpp"
)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_library(teseo SHARED ${SOURCES})

target_include_directories(teseo PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(teseo PRIVATE pthread numa event)  

