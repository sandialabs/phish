#include "phish-bait.h"
#include "phish-bait-common.h"
#include "phish-bait-mpi-common.h"

#include <string.h>
#include <iostream>
#include <stdexcept>
#include <sys/wait.h>

#include <errno.h>

extern "C"
{

int phish_bait_start()
{
  try
  {
    std::vector<std::string> arguments = get_mpiexec_arguments();
    arguments.insert(arguments.begin(), "mpiexec");

    switch(int pid = ::fork())
    {
      case -1:
      {
        throw std::runtime_error(strerror(errno));
      }
      case 0:
      {
        std::vector<char*> argv;
        for(int i = 0; i != arguments.size(); ++i)
          argv.push_back(const_cast<char*>(arguments[i].c_str()));
        argv.push_back(0);

        ::execvp(arguments[0].c_str(), argv.data());
        throw std::runtime_error(strerror(errno)); // Only reached if execvp() fails.
      }
      default:
      {
        int status = 0;
        int options = 0;
        ::waitpid(pid, &status, options);
      }
    }

    return 0;
  }
  catch(std::exception& e)
  {
    std::cerr << "PHISH BAIT ERROR: " << e.what() << std::endl;
    return -1;
  }
}

} // extern "C"

