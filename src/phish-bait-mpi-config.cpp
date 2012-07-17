#include <phish-bait.h>
#include <phish-bait-common.h>
#include <phish-bait-mpi-common.h>

#include <iostream>
#include <stdexcept>

extern "C"
{
 
int phish_bait_start()
{
  try
  {
    std::cerr << "MPICH: mpiexec -configfile <file>" << std::endl;
    std::cerr << "OpenMPI: mpiexec `cat <file>`" << std::endl;

    const std::vector<std::string> arguments = get_mpiexec_arguments();
    for(std::vector<std::string>::const_iterator argument = arguments.begin(); argument != arguments.end(); ++argument)
    {
      std::cout << " " << *argument;
      if(*argument == ":")
        std::cout << "\n";
    }
    std::cout << "\n";

    return 0;
  }
  catch(std::exception& e)
  {
    std::cerr << "PHISH BAIT ERROR: " << e.what() << std::endl;
    return -1;
  }
}

} // extern "C"

