// MINNOW source
// generate hashed messages for sink

#include "stdlib.h"
#include "stdint.h"
#include "stdio.h"
#include "phish.h"

/* ---------------------------------------------------------------------- */

int main(int narg, char **args)
{
  phish_init(&narg,&args);
  phish_input(0,NULL,NULL,1);
  phish_output(0);
  phish_check();

  int idglobal = phish_query("idglobal",0,0);
  printf("PHISH host source %d: %s\n",idglobal,phish_host());

  if (narg != 3) phish_error("Source syntax: source N M");
  int64_t n = atol(args[1]);
  int m = atoi(args[2]);

  int idlocal = phish_query("idlocal",0,0);
  int nlocal = phish_query("nlocal",0,0);
  int nme = n / nlocal;
  if (idlocal < n % nlocal) nme++;

  char *buf = new char[m];
  for (int i = 0; i < m; i++) buf[i] = '\0';
  srand48(12345);
  double rn;

  double time_start = phish_timer();
  
  for (int i = 0; i < nme; i++) {
    phish_pack_raw(buf,m);
    rn = drand48();
    phish_send_key(0,(char *) &rn,sizeof(double));
  }

  phish_close(0);
  phish_loop();

  double time_stop = phish_timer();
  if (idlocal == 0)
    printf("Elapsed time for %ld hashed messages of %d bytes "
           "from %d sources = %g secs\n",
           n,m,nlocal,time_stop-time_start);

  delete [] buf;
  phish_exit();
}
