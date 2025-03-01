project(utils)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

find_package(Boost REQUIRED COMPONENTS program_options)
set_target_properties(Boost::program_options PROPERTIES POSITION_INDEPENDENT_CODE ON)

file(GLOB_RECURSE SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")
file(GLOB_RECURSE HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

add_library(utils SHARED ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(utils PRIVATE ${Boost_LIBRARIES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/../types)
target_link_libraries(utils PRIVATE Boost::program_options)
