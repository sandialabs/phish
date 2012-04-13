#include <phish.hpp>

#include <cassert>

void at_abort(int* cancel)
{
  *cancel = true;
}

int main(int argc, char* argv[])
{
  phish::callback(0, at_abort);

  try
  {
    assert(0 == phish::query("idlocal"));
  }
  catch(...)
  {
    assert(false); // Should never throw.
  }

  try
  {
    phish::query("bogus");
    assert(false); // Should always throw.
  }
  catch(...)
  {
  }

  return 0;
}

