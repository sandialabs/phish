#include <phish.hpp>

#include <functional>
#include <map>
#include <stdexcept>

typedef std::multimap<uint32_t, std::string, std::greater<uint32_t> > items_t;
items_t items;
int count = 0;

void message_callback(int parts)
{
  uint32_t count = phish::unpack<uint32_t>();
  std::string key = phish::unpack<std::string>();
  items.insert(std::make_pair(count, key));
}

void closed_callback()
{
  int i = 0;
  for(items_t::iterator item = items.begin(); (i != count) && (item != items.end()); ++i, ++item)
  {
    phish::pack(item->first);
    phish::pack(item->second);
    phish::send();
  }
  phish::exit();
}

int main(int argc, char* argv[])
{
  phish::init(argc, argv);

  if(argc != 2)
    throw std::runtime_error("Usage: <count>");
  count = ::atoi(argv[1]);

  phish::input(0, message_callback, closed_callback);
  phish::output(0);
  phish::check();
  phish::loop();
}

