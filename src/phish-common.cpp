#include <phish-common.h>

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

