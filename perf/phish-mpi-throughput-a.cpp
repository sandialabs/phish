#include <phish.h>

#include <iomanip>
#include <iostream>
#include <stdexcept>

double start = -1;
int size = 0;
int count = 0;
int received = 0;

void message_callback(int parts)
{
  if(start == -1)
    start = phish_timer();

  char* message = 0;
  int32_t length = 0;
  const int type = phish_unpack(&message, &length);
  if(length != size + 1)
    throw std::runtime_error("message size mismatch");
  received += 1;
  //std::cerr << received << std::endl;
}

int main(int argc, char* argv[])
{
  phish_init(&argc, &argv);

  if(argc != 2)
    throw std::runtime_error("Usage: <size> <count>");
  size = ::atoi(argv[0]);
  count = ::atoi(argv[1]);

  phish_input(0, message_callback, phish_exit, 0);
  phish_check();
  phish_loop();

  const double elapsed = phish_timer() - start;
  const double throughput = count / elapsed;
  const double megabits = (throughput * size * 8.0) / 1000000.0;

  std::cout << elapsed << "," << size << "," << count << "," << throughput << "," << megabits << "\n";

  return 0;
}
