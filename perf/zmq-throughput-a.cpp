#include "wallclock.h"

#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <zmq.hpp>

int main(int argc, char* argv[])
{
  if(argc != 4)
    throw std::runtime_error("Usage: <address> <size> <count>");
  const std::string address = argv[1];
  const int size = ::atoi(argv[2]);
  const int count = ::atoi(argv[3]);

  zmq::context_t context(1);
  zmq::socket_t input(context, ZMQ_PULL);
  input.bind(address.c_str());

  // Don't start timing until we receive our first message ...
  zmq::message_t message;
  input.recv(&message, 0);
  if(message.size() != size)
    throw std::runtime_error("message size mismatch");

  const double start = wallclock();

  for(int i = 0; i != count - 1; ++i)
  {
    zmq::message_t message;
    input.recv(&message, 0);
    if(message.size() != size)
      throw std::runtime_error("message size mismatch");
  }

  const double elapsed = wallclock() - start;
  const double throughput = count / elapsed;
  const double megabits = (throughput * size * 8.0) / 1000000.0;
  std::cout << elapsed << "," << size << "," << count << "," << throughput << "," << megabits << "\n";

  return 0;
}
