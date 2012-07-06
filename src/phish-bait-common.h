#ifndef PHISH_BAIT_COMMON_H
#define PHISH_BAIT_COMMON_H

#include <map>
#include <sstream>
#include <string>
#include <vector>

#define PHISH_BAIT_SEND_PATTERN_BROADCAST "broadcast"
#define PHISH_BAIT_SEND_PATTERN_DIRECT "direct"
#define PHISH_BAIT_SEND_PATTERN_HASHED "hashed"
#define PHISH_BAIT_SEND_PATTERN_ROUND_ROBIN "round-robin"

// Converts any type to std::string
template<typename T>
std::string string_cast(const T& value)
{
  std::ostringstream buffer;
  buffer << value;
  return buffer.str();
}

// Storage for a school
struct school
{
  school(const std::string& _id, const std::vector<std::string>& _hosts, const std::vector<std::string>& _arguments);

  std::string id;
  std::vector<std::string> hosts;
  std::vector<std::string> arguments;
};

// Storage for a hook
struct hook
{
  hook(const std::string& _output_id, int _output_port, const std::string& _style, int _input_port, const std::string& _input_id);

  std::string output_id;
  int output_port;
  std::string style;
  int input_port;
  std::string input_id;
};

// Storage for an individual connection
struct connection
{
  connection(int _output_port, const std::string& _send_pattern, int _input_port, const std::vector<int>& _input_indices);

  // Stores the zero-based output port
  int output_port;
  // Stores the send pattern for the connection
  std::string send_pattern;
  // Stores the zero-based input port
  int input_port;
  // Stores a collection of minnow indices
  std::vector<int> input_indices;
};

// Storage for an individual minnow with connectivity information
struct minnow
{
  minnow(int _school_index, int _local_id);

  int school_index;
  int local_id;
  /// Stores the number of incoming connections per-port
  std::map<int, int> incoming;
  /// Stores outgoing connections
  std::vector<connection> outgoing;
};

// Stores a mapping from school id to school index
extern std::map<std::string, int> g_school_index_map;
// Stores a mapping from school id to minnow index
extern std::map<std::string, std::vector<int> > g_minnow_index_map;

extern std::vector<school> g_schools;
extern std::vector<hook> g_hooks;
extern std::vector<minnow> g_minnows;

#endif // !PHISH_BAIT_COMMON_H
