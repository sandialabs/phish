#include <phish-common.h>

#include <sys/time.h>

void(*g_all_input_ports_closed)() = 0;
void(*g_at_abort)(int*) = 0;

const std::string pop_argument(std::vector<std::string>& arguments)
{
  const std::string argument = arguments.front();
  arguments.erase(arguments.begin());
  return argument;
}

int get_argc(const std::vector<std::string>& arguments)
{
  return arguments.size();
}

char** get_argv(const std::vector<std::string>& arguments)
{
  char** argv = new char*[arguments.size()];
  for(int i = 0; i != arguments.size(); ++i)
  {
    argv[i] = new char[arguments[i].size() + 1];
    strcpy(argv[i], arguments[i].c_str());
  }
  return argv;
}

int phish_abort_internal()
{
  if(!g_at_abort)
    return true;

  int cancel = false;
  g_at_abort(&cancel);
  return !cancel;
}

// Provide default implementations for some parts of the PHISH API ...
extern "C"
{

void phish_callback(void (*done)(), void(*at_abort)(int*))
{
  g_all_input_ports_closed = done;
  g_at_abort = at_abort;
}

double phish_timer()
{
  timeval t;
  ::gettimeofday(&t, 0);
  return t.tv_sec + (t.tv_usec / 1000000.0);
}

} // extern "C"
