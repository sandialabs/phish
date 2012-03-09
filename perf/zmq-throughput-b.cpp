#include <stdexcept>
#include <sstream>
#include <zmq.hpp>

int main(int argc, char* argv[])
{
  if(argc != 5)
    throw std::runtime_error("Usage: <address> <size> <count> <hwm>");
  const std::string address = argv[1];
  const int size = ::atoi(argv[2]);
  const int count = ::atoi(argv[3]);

  uint64_t hwm = 0;
  std::stringstream buffer(argv[4]);
  buffer >> hwm;

  zmq::context_t context(1);
  zmq::socket_t output(context, ZMQ_PUSH);
  output.setsockopt(ZMQ_HWM, &hwm, sizeof(hwm));
  output.connect(address.c_str());

  for(int i = 0; i != count; ++i)
  {
    zmq::message_t message(size);
    output.send(message, 0);
  }

  return 0;
}
