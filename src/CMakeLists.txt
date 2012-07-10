include_directories(${CMAKE_CURRENT_SOURCE_DIR})
install(
  FILES phish.h phish.hpp phish-bait.h phish-bait.hpp
  DESTINATION include
  )

add_library(phish-common STATIC hash.cpp phish-common.cpp)
add_library(phish-bait-common STATIC phish-bait-common.cpp)
add_library(phish-bait-mpi-common STATIC phish-bait-mpi-common.cpp)

if(BUILD_MPI_BACKEND)
  include_directories(${MPI_C_INCLUDE_PATH})

  add_library(phish-mpi SHARED phish-mpi.cpp)
  target_link_libraries(phish-mpi phish-common ${MPI_EXTRA_LIBRARY} ${MPI_LIBRARY})

  install(
    TARGETS phish-mpi
    LIBRARY DESTINATION lib
    )
endif(BUILD_MPI_BACKEND)

if(BUILD_ZMQ_BACKEND)
  include_directories(${ZMQ_INCLUDE_DIR})

  add_library(phish-zmq SHARED phish-zmq.cpp)
  target_link_libraries(phish-zmq phish-common ${ZMQ_LIBRARY})

  add_library(phish-bait-zmq SHARED phish-bait-zmq.cpp)
  target_link_libraries(phish-bait-zmq phish-bait-common ${ZMQ_LIBRARY})

  install(
    TARGETS phish-zmq phish-bait-zmq
    LIBRARY DESTINATION lib
    )
endif(BUILD_ZMQ_BACKEND)

add_library(phish-bait-graphviz SHARED phish-bait-graphviz.cpp)
target_link_libraries(phish-bait-graphviz phish-bait-common)
install(
  TARGETS phish-bait-graphviz
  LIBRARY DESTINATION lib
  )

add_library(phish-bait-mpi-config SHARED phish-bait-mpi-config.cpp)
target_link_libraries(phish-bait-mpi-config phish-bait-common phish-bait-mpi-common)
install(
  TARGETS phish-bait-mpi-config
  LIBRARY DESTINATION lib
  )

add_library(phish-bait-mpi SHARED phish-bait-mpi.cpp)
target_link_libraries(phish-bait-mpi phish-bait-common phish-bait-mpi-common)
install(
  TARGETS phish-bait-mpi
  LIBRARY DESTINATION lib
  )
