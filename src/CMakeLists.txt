include_directories(${CMAKE_CURRENT_SOURCE_DIR})

if(BUILD_MPI_BACKEND)
  include_directories(${MPI_C_INCLUDE_PATH})

  add_library(phish-mpi SHARED hash.cpp phish-mpi.cpp)
  target_link_libraries(phish-mpi ${MPI_EXTRA_LIBRARY} ${MPI_LIBRARY})

  install(
    TARGETS phish-mpi
    LIBRARY DESTINATION lib
    )
endif(BUILD_MPI_BACKEND)

if(BUILD_ZMQ_BACKEND)
  include_directories(${ZMQ_INCLUDE_DIR})

  add_library(phish-zmq SHARED hash.cpp phish-zmq.cpp)
  target_link_libraries(phish-zmq ${ZMQ_LIBRARY})

  add_library(phish-bait-zmq SHARED phish-bait-common.cpp phish-bait-zmq.cpp)
  target_link_libraries(phish-bait-zmq ${ZMQ_LIBRARY})

  install(
    TARGETS phish-zmq phish-bait-zmq
    LIBRARY DESTINATION lib
    )
endif(BUILD_ZMQ_BACKEND)

add_library(phish-bait-graphviz SHARED phish-bait-common.cpp phish-bait-graphviz.cpp)
install(
  TARGETS phish-bait-graphviz
  LIBRARY DESTINATION lib
  )

install(
  FILES phish.h phish.hpp phish-bait.h phish-bait.hpp
  DESTINATION include
  )
