#include <stdexcept>
#include <phish.hpp>

int size = 0;

void message_callback(int parts)
{
  const std::string message = phish::unpack<std::string>();
  if(message.size() != size)
    throw std::runtime_error("message size mismatch");
  phish::pack(message);
  phish::send();
}

int main(int argc, char* argv[])
{
  phish::init(argc, argv);

  if(argc != 3)
    throw std::runtime_error("Usage: <size> <count>");
  size = ::atoi(argv[1]);
  const int count = ::atoi(argv[2]);

  phish::input(0, message_callback, phish::exit);
  phish::output(0);
  phish::check();
  phish::loop();

  return 0;
}
