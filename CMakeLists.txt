cmake_minimum_required(VERSION 2.8)

option(BUILD_MPI_BACKEND "Build the MPI-based backend." TRUE)
option(BUILD_ZMQ_BACKEND "Build the ZMQ-based backend." TRUE)

find_path(ZMQ_INCLUDE_DIR zmq.hpp)
find_library(ZMQ_LIBRARY zmq)
find_package(MPI)

add_subdirectory(src)
add_subdirectory(minnow)
add_subdirectory(minnow2)
add_subdirectory(perf)
add_subdirectory(test)
