#ifndef PHISH_COMMON_H
#define PHISH_COMMON_H

#include <string>
#include <vector>

// Stores the host for this minnow ...
extern std::string g_host;
// Stores the name of the minnow executable ...
extern std::string g_executable;
// Stores the school ID for this minnow ...
extern std::string g_school_id;
// This minnow's ID within the local school ...
extern int g_local_id;
// Number of minnows in the local school ...
extern int g_local_count;
// This minnow's ID within the global net ...
extern int g_global_id;
// Number of minnows in the global net ...
extern int g_global_count;

// Stores a callback to be called when the last input port is closed ...
extern void(*g_all_input_ports_closed)();
// Stores an optional callback to be called by phish_abort() ...
extern void(*g_at_abort)(int*);

/// Removes an argument from the beginning of a collection of arguments.  Preconditions: arguments.empty() == false.
const std::string pop_argument(std::vector<std::string>& arguments);
/// Returns a value suitable for assignment to argc
int get_argc(const std::vector<std::string>& arguments);
/// Returns a value suitable for assignment to argv
char** get_argv(const std::vector<std::string>& arguments);

/// Returns zero to cancel phish_abort(), nonzero to allow the process to exit.
int phish_abort_internal();

void phish_message(const char* type, const char* message);

#endif // !PHISH_COMMON_H
