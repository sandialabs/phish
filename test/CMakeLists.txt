include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(type_conversion type_conversion.cpp)
set_target_properties(type_conversion PROPERTIES COMPILE_FLAGS -Wconversion)

set(API_TESTS
  phish_query
  phish_query_cpp
  )

foreach(API_TEST ${API_TESTS})
  if(BUILD_MPI_BACKEND)
    add_executable(mpi-${API_TEST} ${API_TEST}.cpp)
    target_link_libraries(mpi-${API_TEST} phish-mpi)
    add_test(mpi-${API_TEST} mpi-${API_TEST})
  endif(BUILD_MPI_BACKEND)

  if(BUILD_ZMQ_BACKEND)
    add_executable(zmq-${API_TEST} ${API_TEST}.cpp)
    target_link_libraries(zmq-${API_TEST} phish-zmq)
    add_test(zmq-${API_TEST} zmq-${API_TEST})
  endif(BUILD_ZMQ_BACKEND)
endforeach(API_TEST)

