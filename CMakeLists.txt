cmake_minimum_required(VERSION 3.14)
project(rbenchmark C CXX)
set(CMAKE_CXX_STANDARD 11)

if (POLICY CMP0042)
    cmake_policy (SET CMP0042 NEW)
endif ()

option(BUILD_EXAMPLE "Build example usage" OFF)
option(BENCHMARK_DISABLE "Disable benchmarking" OFF)
set(BUILD_WITH_RBENCHMARK ON PARENT_SCOPE)

add_library(rbenchmark STATIC src/benchmark.cpp)
target_include_directories(rbenchmark PUBLIC "include")


target_compile_definitions(rbenchmark PRIVATE $<$<BOOL:${BENCHMARK_DISABLE}>:BENCHMARK_DISABLE>)
add_compile_definitions(BUILD_WITH_RBENCHMARK)


set_target_properties(rbenchmark PROPERTIES
        VERSION 1.0
        SOVERSION 1
        PUBLIC_HEADER include/roadar/benchmark.hpp)

install(TARGETS rbenchmark
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

add_custom_command(
        TARGET rbenchmark
        POST_BUILD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/header_only
        COMMAND /bin/sh make.sh
)

if (BUILD_EXAMPLE)
    add_executable(bench_example example/simple_benchmark.cpp)
    target_link_libraries(bench_example rbenchmark)
    target_include_directories(bench_example PRIVATE src)

    add_executable(trace_example example/simple_tracing.cpp)
    target_link_libraries(trace_example rbenchmark)
    target_include_directories(trace_example PRIVATE src)
endif()