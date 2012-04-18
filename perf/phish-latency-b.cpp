#include <stdexcept>
#include <phish.hpp>

int size = 0;

void message_callback(int parts)
{
  phish::repack();
  phish::send();
}

int main(int argc, char* argv[])
{
  phish::init(argc, argv);

  if(argc != 3)
    throw std::runtime_error("Usage: <size> <count>");
  size = ::atoi(argv[1]);
  const int count = ::atoi(argv[2]);

  phish::input(0, message_callback, 0, true);
  phish::output(0);
  phish::check();
  phish::loop();
  phish::exit();

  return 0;
}
