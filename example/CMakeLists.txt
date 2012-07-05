include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(word-count word-count.cpp)
target_link_libraries(word-count phish-bait-zmq)

add_executable(hook-styles hook-styles.cpp)
target_link_libraries(hook-styles phish-bait-graphviz)

