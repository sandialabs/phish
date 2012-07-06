#ifndef PHISH_COMMON_H
#define PHISH_COMMON_H

#include <string>
#include <vector>

/// Removes an argument from the beginning of a collection of arguments.  Preconditions: arguments.empty() == false.
const std::string pop_argument(std::vector<std::string>& arguments);
/// Returns a value suitable for assignment to argc
int get_argc(const std::vector<std::string>& arguments);
/// Returns a value suitable for assignment to argv
char** get_argv(const std::vector<std::string>& arguments);

#endif // !PHISH_COMMON_H
