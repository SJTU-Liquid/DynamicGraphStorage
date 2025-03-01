cmake_minimum_required(VERSION 3.10)
project(wrapper)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(OpenMP REQUIRED)
if(OpenMP_FOUND)
    message(STATUS "OpenMP found! Version: ${OpenMP_VERSION}")
    get_cmake_property(_variableNames VARIABLES)
    foreach (_variableName ${_variableNames})
        if (_variableName MATCHES "OpenMP")
            message(STATUS "${_variableName}=${${_variableName}}")
        endif()
    endforeach()
else()
    message(FATAL_ERROR "OpenMP not found!")
endif()


add_compile_options(-O3)


SET(LIBRARIES csr_wrapper.out)

# # # Libraries

# # # ADD_EXECUTABLE(llama_wrapper.out wrapper.h apps/llama_wrapper/llama_wrapper.h apps/llama_wrapper/llama_wrapper.cpp)
# # # TARGET_LINK_LIBRARIES(llama_wrapper.out PUBLIC llama ${OpenMP_CXX_LIBRARIES})

# ADD_EXECUTABLE(livegraph_wrapper.out wrapper.h apps/livegraph/livegraph_wrapper.h apps/livegraph/livegraph_wrapper.cpp)
# TARGET_LINK_LIBRARIES(livegraph_wrapper.out PUBLIC livegraph libcuckoo tbb ${OpenMP_CXX_LIBRARIES})

# ADD_EXECUTABLE(teseo_wrapper.out wrapper.h apps/teseo_wrapper/teseo_wrapper.h apps/teseo_wrapper/teseo_wrapper.cpp)
# TARGET_LINK_LIBRARIES(teseo_wrapper.out PUBLIC teseo numa event_pthreads event_core pthread event ${OpenMP_CXX_LIBRARIES})
# target_compile_definitions(teseo_wrapper.out PRIVATE -DTESEO)

# ADD_EXECUTABLE(sortledton_wrapper.out wrapper.h apps/sortledton_wrapper/sortledton_wrapper.h apps/sortledton_wrapper/sortledton_wrapper.cpp)
# TARGET_LINK_LIBRARIES(sortledton_wrapper.out PUBLIC sortledton tbb ${OpenMP_CXX_LIBRARIES})

# ADD_EXECUTABLE(aspen_wrapper.out wrapper.h apps/aspen_wrapper/aspen_wrapper.h apps/aspen_wrapper/aspen_wrapper.cpp)
# TARGET_LINK_LIBRARIES(aspen_wrapper.out PUBLIC aspen ${OpenMP_CXX_LIBRARIES})


ADD_EXECUTABLE(csr_wrapper.out wrapper.h apps/csr_wrapper/csr_wrapper.h apps/csr_wrapper/csr_wrapper.cpp)
target_compile_definitions(csr_wrapper.out PRIVATE -DENABLE_ITERATOR)

FOREACH(LIB IN LISTS LIBRARIES)
    TARGET_LINK_LIBRARIES(${LIB} PUBLIC  utils atomic ${ITTNOTIFY_LIBRARY} ${OpenMP_CXX_LIBRARIES}  )
    target_include_directories(${LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(${LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
    target_include_directories(${LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../types)
    target_include_directories(${LIB} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../driver)
ENDFOREACH ()


