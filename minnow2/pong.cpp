#include <phish.hpp>

void message_callback(uint32_t parts)
{
  phish::pack(phish::unpack<std::string>());
  phish::send();
}

int main(int argc, char* argv[])
{
  phish::init(argc, argv);
  phish::input(0, message_callback, phish::close);
  phish::output(0);
  phish::check();
  phish::loop();

  return 0;
}

