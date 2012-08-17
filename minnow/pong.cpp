// MINNOW pong
// reflect messages to a sender

#include "stdio.h"
#include "stdlib.h"
#include "phish.h"

void pong(int);

/* ---------------------------------------------------------------------- */

int main(int narg, char **args)
{
  phish_init(&narg,&args);
  phish_input(0,pong,NULL,1);
  phish_output(0);
  phish_check();

  int idglobal = phish_query("idglobal",0,0);
  printf("PHISH host pong %d: %s\n",idglobal,phish_host());

  if (narg != 1) phish_error("Pong syntax: pong");

  phish_loop();
  phish_exit();
}

/* ---------------------------------------------------------------------- */

void pong(int nvalues)
{
  char *buf;
  int len;

  phish_repack();
  phish_send(0);
}
