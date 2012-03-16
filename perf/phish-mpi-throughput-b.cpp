#include <iostream>
#include <stdexcept>
#include <phish.h>

int main(int argc, char* argv[])
{
  phish_init(&argc, &argv);

  if(argc != 2)
    throw std::runtime_error("Usage: <size> <count>");
  const int size = ::atoi(argv[0]);
  const int count = ::atoi(argv[1]);

  phish_output(0);
  phish_check();

  const std::string message(size, '*');
  for(int i = 0; i != count; ++i)
  {
    phish_pack_string(const_cast<char*>(message.c_str()));
    phish_send(0);
  }

  std::cerr << "a" << std::endl;
  phish_exit();
  std::cerr << "b" << std::endl;

  return 0;
}
