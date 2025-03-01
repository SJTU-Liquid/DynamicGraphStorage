cmake_minimum_required(VERSION 3.22)
project(skiplist)
set(CMAKE_BUILD_TYPE Debug)


add_compile_options(-O0 -g)

set(CMAKE_CXX_STANDARD 17)
add_library(m_skiplist STATIC
        skiplist.hpp
        skiplist_test.cpp
        edge_block.hpp
)

add_executable(skiplist_test_neo skiplist_test.cpp )
target_link_libraries(skiplist_test_neo PUBLIC m_skiplist)