#include <stdexcept>

#include <phish.hpp>

int main(int argc, char* argv[])
{
  phish::init(argc, argv);

  if(argc != 3)
    throw std::runtime_error("Usage: <size> <count>");
  const int size = ::atoi(argv[1]);
  const int count = ::atoi(argv[2]);

  phish::output(0);
  phish::check();

  const std::string message(size, '*');
  for(int i = 0; i != count; ++i)
  {
    phish::pack(message);
    phish::send();
  }

  phish::exit();

  return 0;
}
