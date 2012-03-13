#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <phish.hpp>

double start = -1;
int size = 0;
int count = 0;

void message_callback(int parts)
{
  if(start == -1)
    start = phish_timer();

  const std::string message = phish::unpack<std::string>();
  if(message.size() != size)
    throw std::runtime_error("message size mismatch");
}

int main(int argc, char* argv[])
{
  phish::init(argc, argv);

  if(argc != 3)
    throw std::runtime_error("Usage: <size> <count>");
  size = ::atoi(argv[1]);
  count = ::atoi(argv[2]);

  phish::input(0, message_callback, phish::loop_complete);
  phish::check();
  phish::loop();

  const double elapsed = phish_timer() - start;
  const double throughput = count / elapsed;
  const double megabits = (throughput * size * 8.0) / 1000000.0;

  std::cout << elapsed << "," << size << "," << count << "," << throughput << "," << megabits << "\n";

  phish::close();

  return 0;
}
