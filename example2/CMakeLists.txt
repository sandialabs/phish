include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(zmq-word-count zmq-word-count.cpp)
target_link_libraries(zmq-word-count phish-school-zmq)

