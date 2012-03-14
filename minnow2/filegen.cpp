#include <phish.hpp>

int main(int argc, char* argv[])
{
  phish::init(argc, argv);
  phish::output(0);
  phish::check();

  for(int i = 1; i < argc; ++i)
  {
    phish::pack(argv[i]);
    phish::send();
  }

  phish::exit();
}
