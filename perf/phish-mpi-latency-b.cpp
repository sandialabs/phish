#include <iostream>
#include <stdexcept>
#include <phish.h>

int size = 0;

void message_callback(int parts)
{
  phish_repack();
  phish_send(0);
}

int main(int argc, char* argv[])
{
  phish_init(&argc, &argv);

  if(argc != 2)
    throw std::runtime_error("Usage: <size> <count>");
  size = ::atoi(argv[0]);
  const int count = ::atoi(argv[1]);

  phish_input(0, message_callback, 0, 1);
  phish_output(0);
  phish_check();
  phish_loop();
  phish_exit();

  return 0;
}
