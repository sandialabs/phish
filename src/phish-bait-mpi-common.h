#ifndef PHISH_BAIT_MPI_COMMON_H
#define PHISH_BAIT_MPI_COMMON_H

#include <string>
#include <vector>

/// Converts the current phish net into a collection of command-line arguments
/// suitable for use with mpiexec.
std::vector<std::string> get_mpiexec_arguments();

#endif // !PHISH_BAIT_MPI_COMMON_H
