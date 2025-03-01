cmake_minimum_required(VERSION 3.10)

project(aspen)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcilkplus")

include_directories(${PROJECT_SOURCE_DIR}/code/pbbslib)

file(GLOB_RECURSE SOURCES
    "code/algorithms/*.h"
    "code/common/*.h"
    "code/graph/*.h"
    "code/inputs/*.h"
    "code/lib_extensions/*.h"
    "code/pbbslib/*.h"
    "code/trees/*.h"
)

add_library(aspen INTERFACE)

target_include_directories(aspen INTERFACE ${PROJECT_SOURCE_DIR}/code)
target_link_libraries(aspen INTERFACE event pthread numa event_pthreads atomic)
# target_compile_definitions(aspen INTERFACE -DHOMEGROWN)

