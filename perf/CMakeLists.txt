include_directories(${CMAKE_SOURCE_DIR}/src)

# Setup tests using MPI ...
if(BUILD_MPI_BACKEND)
  include_directories(${MPI_C_INCLUDE_PATH})

  add_executable(mpi-latency mpi-latency.cpp)
  target_link_libraries(mpi-latency ${MPI_EXTRA_LIBRARY} ${MPI_LIBRARY})
  configure_file(run-cpp-mpi-latency.py run-cpp-mpi-latency.py)

  add_executable(phish-mpi-latency-a phish-latency-a.cpp)
  target_link_libraries(phish-mpi-latency-a phish-mpi ${MPI_EXTRA_LIBRARY} ${MPI_LIBRARY})
  add_executable(phish-mpi-latency-b phish-latency-b.cpp)
  target_link_libraries(phish-mpi-latency-b phish-mpi ${MPI_EXTRA_LIBRARY} ${MPI_LIBRARY})
  configure_file(run-cpp-phish-mpi-latency.py run-cpp-phish-mpi-latency.py)

  add_executable(phish-mpi-throughput-a phish-throughput-a.cpp)
  target_link_libraries(phish-mpi-throughput-a phish-mpi ${MPI_EXTRA_LIBRARY} ${MPI_LIBRARY})
  add_executable(phish-mpi-throughput-b phish-throughput-b.cpp)
  target_link_libraries(phish-mpi-throughput-b phish-mpi ${MPI_EXTRA_LIBRARY} ${MPI_LIBRARY})
  configure_file(run-cpp-phish-mpi-throughput.py run-cpp-phish-mpi-throughput.py)

  add_executable(phish-mpi-loop phish-loop.cpp)
  target_link_libraries(phish-mpi-loop phish-mpi)
  configure_file(run-cpp-phish-mpi-loop.py run-cpp-phish-mpi-loop.py)

  add_executable(phish-mpi-source phish-source.cpp)
  target_link_libraries(phish-mpi-source phish-mpi)
  add_executable(phish-mpi-map phish-map.cpp)
  target_link_libraries(phish-mpi-map phish-mpi)
  #add_executable(phish-mpi-reduce phish-reduce.cpp)
  #target_link_libraries(phish-mpi-reduce phish-mpi)
  configure_file(run-cpp-phish-mpi-hashed.py run-cpp-phish-mpi-hashed.py)

endif(BUILD_MPI_BACKEND)

# Setup tests using ZMQ ...
if(BUILD_ZMQ_BACKEND)
  include_directories(${ZMQ_INCLUDE_DIR})

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

  add_executable(phish-zmq-latency-a phish-latency-a.cpp)
  target_link_libraries(phish-zmq-latency-a phish-zmq)
  add_executable(phish-zmq-latency-b phish-latency-b.cpp)
  target_link_libraries(phish-zmq-latency-b phish-zmq)
  configure_file(run-cpp-phish-zmq-latency.py run-cpp-phish-zmq-latency.py)

  add_executable(phish-zmq-throughput-a phish-throughput-a.cpp)
  target_link_libraries(phish-zmq-throughput-a phish-zmq)
  add_executable(phish-zmq-throughput-b phish-throughput-b.cpp)
  target_link_libraries(phish-zmq-throughput-b phish-zmq)
  configure_file(run-cpp-phish-zmq-throughput.py run-cpp-phish-zmq-throughput.py)

  add_executable(phish-zmq-loop phish-loop.cpp)
  target_link_libraries(phish-zmq-loop phish-zmq)
  configure_file(run-cpp-phish-zmq-loop.py run-cpp-phish-zmq-loop.py)

  add_executable(phish-zmq-source phish-source.cpp)
  target_link_libraries(phish-zmq-source phish-zmq)
  add_executable(phish-zmq-map phish-map.cpp)
  target_link_libraries(phish-zmq-map phish-zmq)
  #add_executable(phish-zmq-reduce phish-reduce.cpp)
  #target_link_libraries(phish-zmq-reduce phish-zmq)
  configure_file(run-cpp-phish-zmq-hashed.py run-cpp-phish-zmq-hashed.py)

endif(BUILD_ZMQ_BACKEND)

