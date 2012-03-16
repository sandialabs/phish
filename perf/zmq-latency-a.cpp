#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <zmq.hpp>

#include <phish.h>

int main(int argc, char* argv[])
{
  if(argc != 4)
    throw std::runtime_error("Usage: <address> <size> <count>");
  const std::string address = argv[1];
  const int size = ::atoi(argv[2]);
  const int count = ::atoi(argv[3]);

  zmq::context_t context(1);
  zmq::socket_t socket(context, ZMQ_REQ);
  const uint64_t hwm = 100000;
  socket.setsockopt(ZMQ_HWM, &hwm, sizeof(hwm));
  socket.connect(address.c_str());

  const double start = phish_timer();

  for(int i = 0; i != count; ++i)
  {
    zmq::message_t message(size);
    socket.send(message, 0);
    socket.recv(&message, 0);
  }

  const double elapsed = phish_timer() - start;
  const double latency = elapsed / (count * 2.0);

  std::cout << elapsed << "," << size << "," << count << "," << (latency * 1000000.0) << "\n";

  return 0;
}
