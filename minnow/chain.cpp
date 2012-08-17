// MINNOW chain
// send messages from head to tail along a chain
// tail signals head when done

#include "stdlib.h"
#include "stdio.h"
#include "phish.h"

int n,count,tail;
char *buf;

void loop(int);

/* ---------------------------------------------------------------------- */

int main(int narg, char **args)
{
  phish_init(&narg,&args);
  phish_input(0,loop,NULL,1);
  phish_output(0);
  phish_check();

  int idglobal = phish_query("idglobal",0,0);
  printf("PHISH host chain %d: %s\n",idglobal,phish_host());

  if (narg != 3) phish_error("Chain syntax: chain N M");
  n = atoi(args[1]);
  int m = atoi(args[2]);

  if (n == 0) phish_error("N > 0 is required");

  buf = new char[m];
  for (int i = 0; i < m; i++) buf[i] = '\0';
  count = 0;

  int idlocal = phish_query("idlocal",0,0);
  int nlocal = phish_query("nlocal",0,0);
  int head = 0;
  tail = 0;
  if (idlocal == 0) head = 1;
  if (idlocal == nlocal-1) tail = 1;

  if (head) {
    double time_start = phish_timer();
    for (int i = 0; i < n; i++) {
      phish_pack_raw(buf,m);
      phish_send(0);
    }
    phish_loop();
    double time_stop = phish_timer();
    printf("Elapsed time for %d chain messages of %d bytes on "
           "%d procs = %g secs\n",n,m,nlocal,time_stop-time_start);

  } else phish_loop();

  delete [] buf;
  phish_exit();
}

/* ---------------------------------------------------------------------- */

void loop(int nvalues)
{
  if (!tail) {
    phish_repack();
    phish_send(0);
    return;
  }
  count++;
  if (count < n) return;
  phish_close(0);
}
