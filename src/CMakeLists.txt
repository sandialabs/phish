include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if(BUILD_MPI_BACKEND)
  include_directories(${MPI_C_INCLUDE_PATH})

  add_library(phish-mpi SHARED hash.cpp phish-mpi.cpp)
  target_link_libraries(phish-mpi ${MPI_EXTRA_LIBRARY} ${MPI_LIBRARY})
endif(BUILD_MPI_BACKEND)

if(BUILD_ZMQ_BACKEND)
  include_directories(${ZMQ_INCLUDE_DIR})

  add_library(phish-zmq SHARED hash.cpp phish-zmq.cpp)
  target_link_libraries(phish-zmq ${ZMQ_LIBRARY})

  add_library(phish-school-zmq SHARED phish-school-zmq.cpp)
  target_link_libraries(phish-school-zmq ${ZMQ_LIBRARY})
endif(BUILD_ZMQ_BACKEND)
