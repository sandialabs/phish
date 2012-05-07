#include <phish.hpp>
#include <iostream>

int n,count,tail;

void loop(int nvalues)
{
  if(!tail)
  {
    phish::repack();
    phish::send(0);
    return;
  }

  if(++count < n)
    return;

  phish::close(0);
}

/* ---------------------------------------------------------------------- */

int main(int argc, char **argv)
{
  phish::init(argc, argv);
  phish::input(0,loop,NULL,1);
  phish::output(0);
  phish::check();

  if (argc != 3) phish::error("Loop syntax: loop N M");
  n = atoi(argv[1]);
  int m = atoi(argv[2]);

  if (n == 0) phish::error("N > 0 is required");

  const std::string message(m, '*');
  count = 0;

  int idlocal = phish::query("idlocal",0,0);
  int nlocal = phish::query("nlocal",0,0);
  int head = 0;
  tail = 0;
  if (idlocal == 0) head = 1;
  if (idlocal == nlocal-1) tail = 1;

  if (head) {
    double time_start = phish::timer();
    for (int i = 0; i < n; i++) {
      phish::pack(message);
      phish::send(0);
    }
    phish::loop();
    const double elapsed = phish::timer() - time_start;
    const double throughput = n / elapsed;
    const double megabits = (throughput * m * 8.0) / 1000000.0;

    std::cout << elapsed << "," << m << "," << n << "," << throughput << "," << megabits << "," << nlocal << "," << phish::host() << "\n";

  } else phish::loop();

  phish::exit();
}

