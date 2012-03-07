// dummy placeholder for streaming CC algorithm

#include "stdlib.h"
#include "stdio.h"
#include "phish.h"

void datum(int);

/* ---------------------------------------------------------------------- */

enum{ZERO,PRINT_TRIGGER,PRINT,ROTATE_TRIGGER,ROTATE_READY,ROTATE,SHUTDOWN};

/* ---------------------------------------------------------------------- */

int main(int narg, char **args)
{
  phish_init(&narg,&args);
  phish_input(0,datum,NULL,1);
  phish_input(1,datum,NULL,1);
  phish_output(0);
  phish_check();

  if (narg != 1) phish_error("CCdummy syntax: ccdummy E");

  int emax = atoi(args[0]); 

  int me,nprocs;
  phish_school(&me,&nprocs);

  if (emax < nprocs-1) phish_error("Edgegen emax is too small");

  // set bounds for edges I count



  phish_loop();
  phish_exit();
}

/* ---------------------------------------------------------------------- */

void datum(int nvalues)
{
  char *buf;
  int len;

  if (nvalues != 1) phish_error("Count processes one-value datums");

  int type = phish_unpack(&buf,&len);
  if (type != PHISH_STRING) phish_error("Count processes string values");
}
