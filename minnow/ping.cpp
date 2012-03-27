// MINNOW ping
// reflect messages to a receiver

#include "stdlib.h"
#include "stdio.h"
#include "phish.h"

int n,count;
char *buf;

void ping(int);

/* ---------------------------------------------------------------------- */

int main(int narg, char **args)
{
  phish_init(&narg,&args);
  phish_input(0,ping,NULL,1);
  phish_output(0);
  phish_check();

  if (narg != 2) phish_error("Ping syntax: ping N M");
  n = atoi(args[0]);
  int m = atoi(args[1]);

  buf = new char[m];
  for (int i = 0; i < m; i++)
    buf[i] = '\0';
    //    buf[i] = i+1 < m ? '*' : '\0';
  count = 0;

  double time_start = phish_timer();

  phish_pack_raw(buf,m);
  phish_send(0);
  phish_loop();

  double time_stop = phish_timer();
  printf("Elapsed time for %d pingpong exchanges of %d bytes = %g secs\n",
	 n,m,time_stop-time_start);

  delete [] buf;
  phish_exit();
}

/* ---------------------------------------------------------------------- */

void ping(int nvalues)
{
  char *buf;
  int len;

  count++;
  if (count < n) {
    phish_repack();
    phish_send(0);
  } else phish_close(0);
}
