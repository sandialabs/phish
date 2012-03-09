#include <phish.hpp>

int count = 0;
int max_count = 100000;
std::string ping = "";

void message_callback(uint32_t part_count)
{
  count += 1;
  if(count < max_count)
  {
    phish::pack(ping);
    phish::send();
  }
  else
  {
    phish::close_port(0);
  }
}

int main(int argc, char* argv[])
{
  phish::init(argc, argv);
  phish::input(0, message_callback, phish::loop_complete);
  phish::output(0);
  phish::check();

  phish::timer::wallclock timer;

  phish::pack(ping);
  phish::send();
  phish::loop();

  std::cerr << "Elapsed time for " << count << " pingpong of " << ping.size() << " bytes = " << timer.elapsed() << " secs" << std::endl;

  phish::close();
}
