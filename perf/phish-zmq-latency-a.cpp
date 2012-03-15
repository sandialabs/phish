#include <phish.hpp>

#include <iomanip>
#include <iostream>
#include <stdexcept>

int size = 0;
int count = 0;
int received = 0;

void message_callback(int parts)
{
  const std::string message = phish::unpack<std::string>();
  if(message.size() != size)
    throw std::runtime_error("message size mismatch");

  if(++received >= count)
  {
    phish::loop_complete();
    return;
  }

  phish::pack(message);
  phish::send();
}

int main(int argc, char* argv[])
{
  phish::init(argc, argv);

  if(argc != 3)
    throw std::runtime_error("Usage: <size> <count>");
  size = ::atoi(argv[1]);
  count = ::atoi(argv[2]);

  phish::input(0, message_callback, phish::exit);
  phish::output(0);
  phish::check();

  const double start = phish::timer();

  phish::pack(std::string(size, '*'));
  phish::send();
  phish::loop();

  const double elapsed = phish::timer() - start;
  const double latency = elapsed / (count * 2.0);

  std::cout << elapsed << "," << size << "," << count << "," << (latency * 1000000.0) << "\n" << std::flush;

  phish::exit();

  return 0;
}
