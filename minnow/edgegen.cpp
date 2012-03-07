// emit edges as if from external source
// interleave with control messages

#include "mpi.h"
#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "phish.h"

/* ---------------------------------------------------------------------- */

enum{ZERO,PRINT_TRIGGER,PRINT,ROTATE_TRIGGER,ROTATE_READY,ROTATE,SHUTDOWN};

/* ---------------------------------------------------------------------- */

int main(int narg, char **args)
{
  phish_init(&narg,&args);
  phish_output(0);
  phish_check();

  if (narg != 5) phish_error("Edgegen syntax: edgegen E P R S seed");

  int emax = atoi(args[0]); 
  int pfreq = atoi(args[1]); 
  int rfreq = atoi(args[2]); 
  int shutdown = atoi(args[3]); 
  int seed = atoi(args[4]); 

  int me,nprocs;
  phish_school(&me,&nprocs);

  if (emax < nprocs-1) phish_error("Edgegen emax is too small");
  if (pfreq < 1) phish_error("Edgegen pfreq < 1");
  if (rfreq < 1) phish_error("Edgegen rfreq < 1");
  if (shutdown < 1) phish_error("Edgegen shutdown < 1");
  if (seed <= 0) phish_error("Edgegen seed must be positive integer");

  // initialize RN generator

  srand48(seed);

  // initial head is proc 0 in ring

  int head = 0;

  // loop over sending edges
  // edge = random int between 0 and Emax-1 inclusive

  int rn;

  for (int i = 0; i < shutdown; i++) {
    // message to trigger print

    if (i % pfreq == 0) {
      phish_pack_int(-PRINT_TRIGGER);
      phish_send_direct(0,head);
    }

    // message to trigger rotate

    if (i % rfreq == 0) {
      phish_pack_int(-ROTATE_TRIGGER);
      phish_send_direct(0,head);
    }

    // normal edge

    rn = emax*drand48();
    phish_pack_int(rn);
    phish_send_direct(0,head);
  }

  // message to trigger shutdown

  phish_pack_int(-SHUTDOWN);
  phish_send_direct(0,head);

  phish_exit();
}
