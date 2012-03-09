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
  zmq::socket_t socket(context, ZMQ_REP);
  socket.bind(address.c_str());

  for(int i = 0; i != count; ++i)
  {
    zmq::message_t message;
    socket.recv(&message, 0);
    if(message.size() != size)
      throw std::runtime_error("message size mismatch");
    socket.send(message, 0);
  }

  return 0;
}
