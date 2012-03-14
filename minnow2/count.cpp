#include <iostream>
#include <map>
#include <phish.hpp>
#include <tr1/cstdint>

std::map<std::string, uint32_t> counts;

void message_callback(uint32_t parts)
{
  std::string key = phish::unpack<std::string>();
  uint32_t count = phish::unpack<uint32_t>();
  if(!counts.count(key))
    counts[key] = 0;
  counts[key] += count;
}

void closed_callback()
{
  for(std::map<std::string, uint32_t>::iterator count = counts.begin(); count != counts.end(); ++count)
  {
    phish::pack(count->second);
    phish::pack(count->first);
    phish::send();
  }
  phish::exit();
}

int main(int argc, char* argv[])
{
  phish::init(argc, argv);
  phish::input(0, message_callback, closed_callback);
  phish::output(0);
  phish::check();
  phish::loop();
}

