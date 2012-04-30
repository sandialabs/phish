// MINNOW source
// generate hashed messages for map

#include "stdlib.h"
#include "stdio.h"
#include "phish.h"
#include <iostream>

/* ---------------------------------------------------------------------- */

int main(int argc, char **argv)
{
  phish_init(&argc,&argv);
  phish_input(0,NULL,NULL,0);
  phish_output(0);
  phish_check();

  if (argc != 3) phish_error("Source syntax: source N M");
  int n = atoi(argv[1]);
  int m = atoi(argv[2]);

  int idlocal = phish_query("idlocal",0,0);
  int nlocal = phish_query("nlocal",0,0);

  char *buf = new char[m];
  for (int i = 0; i < m; i++) buf[i] = '*';
  srand48(12345);
  double rn;

  double time_start = phish_timer();

  for (int i = 0; i != n; ++i)
  {
    if((i % nlocal) != idlocal)
      continue;

    phish_pack_raw(buf,m);
    rn = drand48();
    phish_send_key(0,(char *) &rn,sizeof(double));
  }

  phish_close(0);
  phish_loop();

  const double elapsed = phish_timer() - time_start;
  const double throughput = n / elapsed;
  const double megabits = (throughput * m * 8.0) / 1000000.0;

  std::cout << elapsed << "," << m << "," << n << "," << throughput << "," << megabits << "," << nlocal << "\n";

  delete [] buf;
  phish_exit();
}
