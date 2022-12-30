cmake_minimum_required(VERSION 3.14)
project(rbenchmark C CXX)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Globally notify about lib
set(BUILD_WITH_RBENCHMARK ON PARENT_SCOPE)
add_compile_definitions(BUILD_WITH_RBENCHMARK)

if (POLICY CMP0042)
    cmake_policy(SET CMP0042 NEW)
endif ()

option(BUILD_EXAMPLE "Build example usage" OFF)
option(BUILD_HEADER_ONLY "Build header only" OFF)
option(BENCHMARK_DISABLED "Disable benchmarking" OFF)
option(NO_INSTALL "Disable Install (windows only)" OFF)

if(NOT TARGET rbenchmark)
    add_library(rbenchmark STATIC src/benchmark.cpp)
endif()

target_include_directories(rbenchmark PUBLIC "include")

target_compile_definitions(rbenchmark PRIVATE $<$<BOOL:${BENCHMARK_DISABLED}>:BENCHMARK_DISABLED>)

set_target_properties(rbenchmark PROPERTIES
        VERSION 1.0
        SOVERSION 1
        PUBLIC_HEADER include/roadar/benchmark.hpp)

if(NOT MSVC AND NO_INSTALL)
    message(FATAL_ERROR "NO_INSTALL is for Windows only!")
endif()

if(NO_INSTALL) # for windows install might not work because of access permissions
    message(STATUS "Library will be built in folder ${CMAKE_CURRENT_BINARY_DIR}, please copy include and lib folders to your project")
    # Copy the needed header into the build directory
    add_custom_command(
        TARGET rbenchmark POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${PROJECT_SOURCE_DIR}/include/roadar/benchmark.hpp
        ${CMAKE_CURRENT_BINARY_DIR}/include/roadar/benchmark.hpp)
    # Copy the needed lib files (and pdb in case of debug) to its config directory
    add_custom_command(
        TARGET rbenchmark POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_BINARY_DIR}/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>"
        "${CMAKE_CURRENT_BINARY_DIR}/lib/$<$<CONFIG:Debug>:Debug>$<$<CONFIG:Release>:Release>")
else()
    install(TARGETS rbenchmark
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()



if (BUILD_HEADER_ONLY)
    if(MSVC)
        message(FATAL_ERROR "BUILD_HEADER_ONLY is not supported for Windows")
    endif()
    add_custom_command(
            TARGET rbenchmark
            POST_BUILD
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/header_only
            COMMAND /bin/sh make.sh
    )
endif ()

if (BUILD_EXAMPLE)
    set(CMAKE_CXX_FLAGS -pthread)
    add_executable(bench_example example/simple_benchmark.cpp)
    target_link_libraries(bench_example rbenchmark)
    target_include_directories(bench_example PRIVATE src)

    add_executable(trace_example example/simple_tracing.cpp)
    target_link_libraries(trace_example rbenchmark)
    target_include_directories(trace_example PRIVATE src)
endif ()