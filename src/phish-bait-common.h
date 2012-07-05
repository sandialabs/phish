#ifndef PHISH_BAIT_COMMON_H
#define PHISH_BAIT_COMMON_H

#include <map>
#include <sstream>
#include <string>
#include <vector>

// Converts any type to std::string
template<typename T>
std::string string_cast(const T& value)
{
  std::ostringstream buffer;
  buffer << value;
  return buffer.str();
}

// Storage for a hook specification
struct connection
{
  connection(int om, int op, const char* sp, int ip, const std::vector<int>& im) :
    output_minnow(om),
    output_port(op),
    send_pattern(sp),
    input_port(ip),
    input_minnows(im)
  {
  }

  int output_minnow;
  int output_port;
  std::string send_pattern;
  int input_port;
  std::vector<int> input_minnows;
};

// Stores a mapping from minnow id to indices
extern std::map<std::string, std::vector<int> > g_id_map;

// Stores a per-minnow id
extern std::vector<std::string> g_ids;
// Stores a zero-based per-minnow local id
extern std::vector<int> g_local_ids;
// Stores a per-minnow count of the number of minnows in a school
extern std::vector<int> g_local_counts;
// Stores a per-minnow host specification, which could be an empty string
extern std::vector<std::string> g_hosts;
// Stores a per-minnow set of command line arguments
extern std::vector<std::vector<std::string> > g_arguments;

// Stores a set of hook specifications
extern std::vector<connection> g_connections;

#endif // !PHISH_BAIT_COMMON_H
