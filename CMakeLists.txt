project(driver)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3 -mcx16")

# add_subdirectory(libraries/teseo)
# add_subdirectory(libraries/sortledton)
# add_subdirectory(libraries/LiveGraph)
# add_subdirectory(libraries/aspen)
# add_subdirectory(libraries/llama)
add_subdirectory(utils)
add_subdirectory(third-party/libcuckoo)
add_subdirectory(wrapper)
add_subdirectory(container)
add_subdirectory(edge_wrapper)
add_subdirectory(generator)
add_subdirectory(types)