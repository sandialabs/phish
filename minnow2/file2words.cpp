#include <fstream>
#include <iostream>
#include <phish.hpp>

void message_callback(uint32_t parts)
{
  const std::string filename = phish::unpack<std::string>();
  std::ifstream stream(filename.c_str());
  std::string word;
  while(true)
  {
    stream >> word;
    if(!stream)
      break;
    phish::pack(word);
    phish::pack(static_cast<uint32_t>(1));
    phish::send();
  }
}

int main(int argc, char* argv[])
{
  phish::init(argc, argv);
  phish::input(0, message_callback, phish::exit);
  phish::output(0);
  phish::check();
  phish::loop();
}

