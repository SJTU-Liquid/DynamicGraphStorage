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
endif ()

# add_definitions(-DBLOCK_SIZE_VALUE=256)
# add_definitions(-DBLOOM_FILTER_BITS_VALUE=24)
# add_definitions(-DPMA_LEAF_UPPER_BOUND_VALUE=256)
add_definitions(-DDEFAULT_VECTOR_SIZE_VALUE=0)

function(calculate_power base exponent result)
    set(value 1)
    foreach(i RANGE ${exponent})
        math(EXPR value "${value} * ${base}")
    endforeach()
    set(${result} ${value} PARENT_SCOPE)
endfunction()

foreach(BIT_SIZE RANGE 4 10)
    set(SUFFIX _${BIT_SIZE})
    math(EXPR FIXED_BIT_SIZE "${BIT_SIZE}-1")
    calculate_power(2 ${FIXED_BIT_SIZE} BLOCK_SIZE_VALUE)
    message(STATUS "Configuring for BIT_SIZE=${BIT_SIZE}, BLOCK_SIZE_VALUE=${BLOCK_SIZE_VALUE}")

    add_executable(sorted_array_edge_test${SUFFIX} apps/sorted_array_wrapper.cpp edge_driver_main.h edge_driver.h edge_wrapper.h)
    target_compile_definitions(sorted_array_edge_test${SUFFIX} PRIVATE -DENABLE_TIMESTAMP)
    target_compile_definitions(sorted_array_edge_test${SUFFIX} PUBLIC -DBLOCK_SIZE_VALUE=${BLOCK_SIZE_VALUE})

    add_executable(pam_edge_test${SUFFIX} apps/pam_wrapper.cpp edge_driver_main.h edge_driver.h edge_wrapper.h)
    target_compile_definitions(pam_edge_test${SUFFIX} PUBLIC -DBLOCK_SIZE_VALUE=${BLOCK_SIZE_VALUE})
    target_include_directories(pam_edge_test${SUFFIX} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../container/utils/PAM/_deps/parlaylib-src/include)

    add_executable(neo_pam_edge_test${SUFFIX} apps/neo_pam_wrapper.cpp edge_driver_main.h edge_driver.h edge_wrapper.h)
    target_compile_definitions(neo_pam_edge_test${SUFFIX} PUBLIC -DBLOCK_SIZE_VALUE=${BLOCK_SIZE_VALUE})
    target_include_directories(neo_pam_edge_test${SUFFIX} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../container/utils/PAM/_deps/parlaylib-src/include)

    add_executable(logblock_edge_test${SUFFIX} apps/logblock_wrapper.cpp edge_driver_main.h edge_driver.h edge_wrapper.h)
    target_compile_definitions(logblock_edge_test${SUFFIX} PRIVATE -DENABLE_TIMESTAMP)
    target_compile_definitions(logblock_edge_test${SUFFIX} PUBLIC -DBLOCK_SIZE_VALUE=${BLOCK_SIZE_VALUE})

    add_executable(skiplist_edge_test${SUFFIX} apps/skiplist_wrapper.cpp edge_driver_main.h edge_driver.h edge_wrapper.h)
    target_compile_definitions(skiplist_edge_test${SUFFIX} PUBLIC -DENABLE_TIMESTAMP)
    target_compile_definitions(skiplist_edge_test${SUFFIX} PUBLIC -DBLOCK_SIZE_VALUE=${BLOCK_SIZE_VALUE})

    add_executable(pma_edge_test${SUFFIX} apps/pma_wrapper.cpp edge_driver_main.h edge_driver.h edge_wrapper.h)
    target_link_libraries(pma_edge_test${SUFFIX} PUBLIC)
    target_compile_definitions(pma_edge_test${SUFFIX} PUBLIC -DENABLE_TIMESTAMP)
    target_compile_definitions(pma_edge_test${SUFFIX} PUBLIC -DBLOCK_SIZE_VALUE=${BLOCK_SIZE_VALUE})

    add_executable(sorted_array_edge_test_unversioned${SUFFIX} apps/sorted_array_wrapper.cpp edge_driver_main.h edge_driver.h edge_wrapper.h)
    target_compile_definitions(sorted_array_edge_test_unversioned${SUFFIX} PUBLIC -DBLOCK_SIZE_VALUE=${BLOCK_SIZE_VALUE})

    add_executable(logblock_edge_test_unversioned${SUFFIX} apps/logblock_wrapper.cpp edge_driver_main.h edge_driver.h edge_wrapper.h)
    target_compile_definitions(logblock_edge_test_unversioned${SUFFIX} PUBLIC -DBLOCK_SIZE_VALUE=${BLOCK_SIZE_VALUE})

    add_executable(skiplist_edge_test_unversioned${SUFFIX} apps/skiplist_wrapper.cpp edge_driver_main.h edge_driver.h edge_wrapper.h)
    target_link_libraries(skiplist_edge_test_unversioned${SUFFIX} PUBLIC)
    target_compile_definitions(skiplist_edge_test_unversioned${SUFFIX} PRIVATE -DBLOCK_SIZE_VALUE=${BLOCK_SIZE_VALUE})

    add_executable(pma_edge_test_unversioned${SUFFIX} apps/pma_wrapper.cpp edge_driver_main.h edge_driver.h edge_wrapper.h)
    target_link_libraries(pma_edge_test_unversioned${SUFFIX} PUBLIC)
    target_compile_definitions(pma_edge_test_unversioned${SUFFIX} PRIVATE -DBLOCK_SIZE_VALUE=${BLOCK_SIZE_VALUE})

    foreach (EXE sorted_array_edge_test${SUFFIX} pam_edge_test${SUFFIX} neo_pam_edge_test${SUFFIX} logblock_edge_test${SUFFIX}
                  skiplist_edge_test${SUFFIX} pma_edge_test${SUFFIX} sorted_array_edge_test_unversioned${SUFFIX}
                  logblock_edge_test_unversioned${SUFFIX} skiplist_edge_test_unversioned${SUFFIX} pma_edge_test_unversioned${SUFFIX})
        target_link_libraries(${EXE} PUBLIC utils  ${ITTNOTIFY_LIBRARY} pthread)
        target_include_directories(${EXE} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
        target_include_directories(${EXE} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/edge_wrapper)
        target_include_directories(${EXE} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
        target_include_directories(${EXE} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../container)
        target_include_directories(${EXE} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../utils)
        target_include_directories(${EXE} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../types)
    endforeach()
endforeach()