// MINNOW reduce
// read hashed messages from map or source

#include "stdlib.h"
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
  printf("PHISH host reduce %d: %s\n",idglobal,phish_host());

  if (narg != 1) phish_error("Reduce syntax: reduce");

  phish_loop();
  phish_exit();
}
