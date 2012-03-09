#include <iostream>
#include <stdexcept>
#include <phish.h>

int size = 0;

void message_callback(int parts)
{
  char* message = 0;
  int length = 0;
  const int type = phish_unpack(&message, &length);

  if(length != size + 1)
  {
    std::cerr << length << " <> " << size << std::endl;
    throw std::runtime_error("message size mismatch");
  }

  phish_pack_string(message);
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
