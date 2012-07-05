include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(wc wc.cpp)
#target_link_libraries(zmq-word-count phish-bait-zmq)
target_link_libraries(wc phish-bait-graphviz)

