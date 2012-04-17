#include <phish.h>

#include <iomanip>
#include <iostream>
#include <stdexcept>

int size = 0;
int count = 0;
int received = 0;

void message_callback(int parts)
{
  char* message = 0;
  int32_t length = 0;
  const int type = phish_unpack(&message, &length);
  if(length != size + 1)
    throw std::runtime_error("message size mismatch");

  if(++received < count)
  {
    phish_pack_string(message);
    phish_send(0);
  }
  else
  {
    phish_close(0);
  }
}

int main(int argc, char* argv[])
{
  phish_init(&argc, &argv);

  if(argc != 2)
    throw std::runtime_error("Usage: <size> <count>");
  size = ::atoi(argv[0]);
  count = ::atoi(argv[1]);

  phish_input(0, message_callback, 0, 1);
  phish_output(0);
  phish_check();

  const double start = phish_timer();

  std::string message(size, '*');
  phish_pack_string(const_cast<char*>(message.c_str()));
  phish_send(0);
  phish_loop();

  const double elapsed = phish_timer() - start;
  const double latency = elapsed / (count * 2.0);

  std::cout << elapsed << "," << size << "," << count << "," << (latency * 1000000.0) << "\n" << std::flush;

  phish_exit();

  return 0;
}
