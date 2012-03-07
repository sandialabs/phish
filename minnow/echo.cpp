// echo strings read from stdin to stdout, one line at a time

#include "stdlib.h"
#include "string.h"
#include "stdio.h"

#define MAXLINE 1024

/* ---------------------------------------------------------------------- */

int main(int narg, char **args)
{
  char line[MAXLINE];
  char *ptr;

  while (ptr = fgets(line,MAXLINE,stdin)) printf("%s",line);
}
