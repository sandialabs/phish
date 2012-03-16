include_directories(${CMAKE_SOURCE_DIR}/src)
include_directories(${ZMQ_INCLUDE_DIR})

# Setup tests using MPI ...
if(BUILD_MPI_BACKEND)

  add_executable(phish-mpi-latency-a phish-mpi-latency-a.cpp)
  target_link_libraries(phish-mpi-latency-a phish-mpi ${MPI_EXTRA_LIBRARY} ${MPI_LIBRARY})

  add_executable(phish-mpi-latency-b phish-mpi-latency-b.cpp)
  target_link_libraries(phish-mpi-latency-b phish-mpi ${MPI_EXTRA_LIBRARY} ${MPI_LIBRARY})

  configure_file(run-cpp-phish-mpi-latency.py run-cpp-phish-mpi-latency.py)

endif(BUILD_MPI_BACKEND)

# Setup tests using ZMQ ...
if(BUILD_ZMQ_BACKEND)

  configure_file(run-py-zmq-latency.py run-py-zmq-latency.py)

  configure_file(run-py-zmq-throughput.py run-py-zmq-throughput.py)

  add_executable(zmq-latency-a zmq-latency-a.cpp)
  target_link_libraries(zmq-latency-a ${ZMQ_LIBRARY} phish-zmq)

  add_executable(zmq-latency-b zmq-latency-b.cpp)
  target_link_libraries(zmq-latency-b ${ZMQ_LIBRARY})

  configure_file(run-cpp-zmq-latency.py run-cpp-zmq-latency.py)

  add_executable(zmq-throughput-a zmq-throughput-a.cpp)
  target_link_libraries(zmq-throughput-a ${ZMQ_LIBRARY} phish-zmq)

  add_executable(zmq-throughput-b zmq-throughput-b.cpp)
  target_link_libraries(zmq-throughput-b ${ZMQ_LIBRARY})

  configure_file(run-cpp-zmq-throughput.py run-cpp-zmq-throughput.py)

  add_executable(phish-zmq-latency-a phish-zmq-latency-a.cpp)
  target_link_libraries(phish-zmq-latency-a phish-zmq)

  add_executable(phish-zmq-latency-b phish-zmq-latency-b.cpp)
  target_link_libraries(phish-zmq-latency-b phish-zmq)

  configure_file(run-cpp-phish-zmq-latency.py run-cpp-phish-zmq-latency.py)

  add_executable(phish-zmq-throughput-a phish-zmq-throughput-a.cpp)
  target_link_libraries(phish-zmq-throughput-a phish-zmq)

  add_executable(phish-zmq-throughput-b phish-zmq-throughput-b.cpp)
  target_link_libraries(phish-zmq-throughput-b phish-zmq)

  configure_file(run-cpp-phish-zmq-throughput.py run-cpp-phish-zmq-throughput.py)

endif(BUILD_ZMQ_BACKEND)

