#include <phish.h>

#include <assert.h>

void at_abort(int* cancel)
{
  *cancel = true;
}

int main(int argc, char* argv[])
{
  phish_callback(0, at_abort);
  assert(0 == phish_query("idlocal", 0, 0));
  assert(-1 == phish_query("bogus", 0,  0));

  return 0;
}

