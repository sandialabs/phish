#include <phish-bait.h>
#include <phish-bait-common.h>

int phish_bait_start()
{
  // This loop is here to force linking ... it doesn't do anything useful.
  for(std::vector<minnow>::iterator minnow = g_minnows.begin(); minnow != g_minnows.end(); ++minnow)
  {
  }
  return 0;
}

