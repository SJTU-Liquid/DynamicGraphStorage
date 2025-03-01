cmake_minimum_required(VERSION 3.10)
project(container)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

find_package(OpenMP)
if(OpenMP_FOUND)
    message(STATUS "OpenMP found!")
else()
    message(FATAL_ERROR "OpenMP not found!")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mavx2 -mavx")

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Release mode")
    add_compile_options(-O3)
else ()
    message(STATUS "Debug mode")
    add_compile_options(-O0 -g)
    # set(CMAKE_CXX_CLANG_TIDY clang-tidy -checks=-*,clang-analyzer-*)
endif ()


add_definitions(-DBLOCK_SIZE_VALUE=256)
add_definitions(-DBLOOM_FILTER_BITS_VALUE=24)
add_definitions(-DPMA_LEAF_UPPER_BOUND_VALUE=128)
add_definitions(-DDEFAULT_VECTOR_SIZE_VALUE=0)

add_subdirectory(utils/PAM)
add_subdirectory(utils/unordered_dense)
# add_subdirectory(utils/neo_skiplist)
add_subdirectory(edge_index/skiplist)

add_library(bloom_filter SHARED utils/bloom_filter/hashutil.h utils/bloom_filter/simd-block-fixed-fpp.hpp
        utils/bloom_filter/simd-block-fixed-fpp.cpp)

# Vertex Index
add_library(vector_vertex_index SHARED vertex_index/vertex_index.cpp vertex_index/vector.hpp)
# add_library(rb_tree_vertex_index SHARED vertex_index/vertex_index.cpp vertex_index/rb_tree.hpp)
add_library(hashmap_vertex_index SHARED vertex_index/vertex_index.cpp vertex_index/hashmap.hpp)
# add_library(art_tree_vertex_index SHARED vertex_index/vertex_index.cpp vertex_index/art_tree.hpp utils/art/art_wrapper.h utils/art/art_wrapper.c utils/art/art.c utils/art/art.h)
# add_library(teseo_like_vertex_index SHARED vertex_index/vertex_index.cpp vertex_index/teseo_like.hpp utils/art/art_wrapper.h utils/art/art_wrapper.c utils/art/art.c utils/art/art.h)
target_link_libraries(hashmap_vertex_index PUBLIC tbb)
# target_link_libraries(teseo_like_vertex_index PUBLIC tbb)

# Edge Index
# add_library(vector_edge_index SHARED edge_index/edge_index.cpp edge_index/vector.hpp)
# add_library(rb_tree_edge_index SHARED edge_index/edge_index.cpp edge_index/rb_tree.hpp)
# add_library(hashmap_edge_index SHARED edge_index/edge_index.cpp edge_index/hashmap.hpp)
add_library(logblock_edge_index SHARED edge_index/edge_index.cpp edge_index/log_block.hpp)
add_library(pma_edge_index SHARED edge_index/edge_index.cpp edge_index/pma.hpp)
# target_link_libraries(hashmap_edge_index PUBLIC unordered_dense)

SET(EXECUTABLE
    vector2pma 
    avl_tree2pam_cow avl_tree2pam_cow_p avl_tree2pam_cow_flat
    vector2skiplist 
    vector2sorted_array 
    vector2logblock 
    vector2pma_unversioned vector2skiplist_unversioned vector2sorted_array_unversioned vector2logblock_unversioned
    vector2skiplist_adaptive
)

# Instances
# versioned
add_executable(vector2skiplist wrapper.h apps/vector2skiplist.cpp)
target_compile_definitions(vector2skiplist PUBLIC -DENABLE_TIMESTAMP -DENABLE_LOCK -DENABLE_ADAPTIVE -DENABLE_ITERATOR)
target_compile_definitions(vector2skiplist PUBLIC -DBLOCK_SIZE_VALUE=256 -DDEFAULT_VECTOR_SIZE_VALUE=256)

add_executable(vector2logblock apps/vector2logblock.cpp)
target_link_libraries(vector2logblock PUBLIC vector_vertex_index logblock_edge_index)
target_compile_definitions(vector2logblock PRIVATE -DENABLE_TIMESTAMP -DENABLE_LOCK -DENABLE_ITERATOR)
target_compile_definitions(vector2logblock PUBLIC -DBLOCK_SIZE_VALUE=256)

add_executable(avl_tree2pam_cow wrapper.h apps/avl2pam_cow/avl2pam_cow.cpp)
target_link_libraries(avl_tree2pam_cow PUBLIC pam unordered_dense)
target_compile_definitions(avl_tree2pam_cow PUBLIC -DBLOCK_SIZE_VALUE=256 -DENABLE_ITERATOR)

add_executable(avl_tree2pam_cow_p wrapper.h apps/avl2pam_cow/avl2pam_cow.cpp)
target_link_libraries(avl_tree2pam_cow_p PUBLIC pam unordered_dense)
target_compile_definitions(avl_tree2pam_cow_p PUBLIC -DBLOCK_SIZE_VALUE=256 -DENABLE_ITERATOR -DENABLE_PARLAY)


add_executable(avl_tree2pam_cow_flat wrapper.h apps/avl2pam_cow/avl2pam_cow.cpp)
target_link_libraries(avl_tree2pam_cow_flat PUBLIC pam unordered_dense)
target_compile_definitions(avl_tree2pam_cow_flat PUBLIC -DBLOCK_SIZE_VALUE=256 -DENABLE_ITERATOR -DENABLE_PARLAY -DENABLE_FLAT_SNAPSHOT)

add_executable(vector2sorted_array wrapper.h apps/vector2sorted_array.cpp)
target_compile_definitions(vector2sorted_array PRIVATE -DENABLE_TIMESTAMP -DENABLE_LOCK -DENABLE_ITERATOR)
target_compile_definitions(vector2sorted_array PUBLIC -DBLOCK_SIZE_VALUE=256)

add_executable(vector2pma wrapper.h apps/vector2pma.cpp)
target_link_libraries(vector2pma PUBLIC tbb unordered_dense)
target_compile_definitions(vector2pma PUBLIC -DENABLE_TIMESTAMP -DENABLE_LOCK -DENABLE_ITERATOR)
target_compile_definitions(vector2pma PUBLIC -DBLOCK_SIZE_VALUE=256 -DDEFAULT_VECTOR_SIZE_VALUE=0)

# unversioned
add_executable(vector2skiplist_unversioned wrapper.h apps/vector2skiplist.cpp)
target_link_libraries(vector2skiplist_unversioned PUBLIC vector_vertex_index)
target_compile_definitions(vector2skiplist_unversioned PRIVATE -DDEFAULT_VECTOR_SIZE_VALUE=0 -DBLOCK_SIZE_VALUE=256 -DENABLE_ITERATOR)

add_executable(vector2skiplist_adaptive wrapper.h apps/vector2skiplist.cpp)
target_link_libraries(vector2skiplist_adaptive PUBLIC vector_vertex_index)
target_compile_definitions(vector2skiplist_adaptive PUBLIC -DENABLE_ADAPTIVE -DENABLE_ITERATOR)
target_compile_definitions(vector2skiplist_adaptive PRIVATE -DDEFAULT_VECTOR_SIZE_VALUE=256 -DBLOCK_SIZE_VALUE=256)

add_executable(vector2logblock_unversioned apps/vector2logblock.cpp )
target_link_libraries(vector2logblock_unversioned PUBLIC vector_vertex_index logblock_edge_index)
target_compile_definitions(vector2logblock_unversioned PRIVATE -DDEFAULT_VECTOR_SIZE_VALUE=0 -DBLOCK_SIZE_VALUE=256 -DENABLE_ITERATOR)

add_executable(vector2sorted_array_unversioned wrapper.h apps/vector2sorted_array.cpp)
target_compile_definitions(vector2sorted_array_unversioned PRIVATE -DDEFAULT_VECTOR_SIZE_VALUE=0 -DBLOCK_SIZE_VALUE=256 -DENABLE_ITERATOR)

add_executable(vector2pma_unversioned wrapper.h apps/vector2pma.cpp)
target_link_libraries(vector2pma_unversioned PUBLIC tbb unordered_dense)
target_compile_definitions(vector2pma_unversioned PRIVATE -DDEFAULT_VECTOR_SIZE_VALUE=0 -DBLOCK_SIZE_VALUE=256 -DENABLE_ITERATOR)

# Dependencies
FOREACH (EXE IN LISTS EXECUTABLE)
    target_link_libraries(${EXE} PUBLIC tbb utils atomic pthread ${OpenMP_CXX_LIBRARIES} ${ITTNOTIFY_LIBRARY} )
    target_include_directories(${EXE} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(${EXE} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
    target_include_directories(${EXE} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../types)
    target_include_directories(${EXE} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/utils/PAM/_deps/parlaylib-src/include)
    target_include_directories(${EXE} PUBLIC ${OpenMP_INCLUDE_DIRS})
    target_include_directories(${EXE} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../driver)
ENDFOREACH ()

set(VERTEX_TEST vector_vertex_test avl_vertex_test hashmap_vertex_test)
add_executable(vector_vertex_test vertex_wrapper/apps/vector_wrapper.cpp)
add_executable(hashmap_vertex_test vertex_wrapper/apps/hashtable_wrapper.cpp)

add_executable(avl_vertex_test vertex_wrapper/apps/avl_wrapper.cpp)
target_link_libraries(avl_vertex_test PUBLIC pam unordered_dense)

FOREACH (EXE IN LISTS VERTEX_TEST)
    target_link_libraries(${EXE} PUBLIC utils ${ITTNOTIFY_LIBRARY} )
    target_include_directories(${EXE} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(${EXE} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/vertex_wrapper)
    target_include_directories(${EXE} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
    target_include_directories(${EXE} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/utils)
    target_include_directories(${EXE} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/utils/PAM/_deps/parlaylib-src/include)
ENDFOREACH ()