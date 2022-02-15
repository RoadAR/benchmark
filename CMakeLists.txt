cmake_minimum_required(VERSION 3.14)
project(rbenchmark C CXX)
set(CMAKE_CXX_STANDARD 11)

if (POLICY CMP0042)
    cmake_policy (SET CMP0042 NEW)
endif ()

add_library(rbenchmark STATIC src/benchmark.cpp)
target_include_directories(rbenchmark PUBLIC "include")

set_target_properties(rbenchmark PROPERTIES
        VERSION 1.0
        SOVERSION 1
        PUBLIC_HEADER include/roadar/benchmark.hpp)

install(TARGETS rbenchmark
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})


option(BUILD_EXAMPLE "Build example usage" OFF)

if (BUILD_EXAMPLE)
    add_executable(example example/simple_benchmark.cpp)
    target_link_libraries(example rbenchmark)
    target_include_directories(example PRIVATE src)
endif()