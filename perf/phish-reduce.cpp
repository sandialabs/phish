// MINNOW reduce
// read hashed messages from map

#include "stdlib.h"
#include "stdio.h"
#include "phish.h"

/* ---------------------------------------------------------------------- */

int main(int argc, char **argv)
{
  phish_init(&argc,&argv);
  phish_input(0,NULL,NULL,1);
  phish_output(0);
  phish_check();

  if (argc != 1) phish_error("Reduce syntax: reduce");

  phish_loop();
  phish_exit();
}
