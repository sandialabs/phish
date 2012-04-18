#include <phish.hpp>
#include <iostream>

void loop(int nvalues)
{
  // Not the head of the loop ...
  if(0 != phish::query("idlocal"))
  {
    phish::repack();
    phish::send();
  }
}

int main(int argc, char* argv[])
{
  phish::init(argc, argv);

  if(argc != 3)
    phish::error("Usage: <message size> <message count>");

  const int size = atoi(argv[1]);
  const int count = atoi(argv[2]);

  if(count == 0)
    phish::error("message count > 0 is required");

  phish::input(0, loop, 0, false);
  phish::output(0);
  phish::check();

  // Head of the loop ...
  if(0 == phish::query("idlocal"))
  {
    const double start = phish::timer();
    const std::string message(size, '*');
    for(int i = 0; i != count; ++i)
    {
      phish::pack(message);
      phish::send();
    }
    phish::close();
    phish::loop();
    const double elapsed = phish::timer() - start;
    const double throughput = count / elapsed;
    const double megabits = (throughput * size * 8.0) / 1000000.0;

    std::cout << elapsed << "," << size << "," << count << "," << throughput << "," << megabits << "," << phish::query("nlocal") << "\n";
  }
  // Not the head of the loop ...
  else
  {
    phish::loop();
  }

  phish::exit();

  return 0;
}

