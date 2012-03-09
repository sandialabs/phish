#include "wallclock.h"
#include <sys/time.h>

double wallclock()
{
  timeval t;
  ::gettimeofday(&t, 0);

  return t.tv_sec + (t.tv_usec / 1000000.0);
}

