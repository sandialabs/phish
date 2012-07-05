#include <phish-bait.h>
#include <phish-bait-common.h>

#include <iostream>
#include <stdexcept>

std::map<std::string, std::vector<int> > g_id_map;
std::vector<std::string> g_ids;
std::vector<int> g_local_ids;
std::vector<int> g_local_counts;
std::vector<std::string> g_hosts;
std::vector<std::vector<std::string> > g_arguments;
std::vector<connection> g_connections;

void phish_bait_reset()
{
  g_id_map.clear();

  g_ids.clear();
  g_local_ids.clear();
  g_local_counts.clear();
  g_hosts.clear();
  g_arguments.clear();

  g_connections.clear();
}

int phish_bait_minnows(const char* id, int count, const char** hosts, int argc, const char** argv)
{
  try
  {
    if(g_id_map.count(id))
      throw std::runtime_error("Duplicate minnow id");
    if(count < 1)
      throw std::runtime_error("Minnow count must be >= 1");

    g_id_map[id] = std::vector<int>();

    for(int i = 0; i != count; ++i)
    {
      g_id_map[id].push_back(g_ids.size());

      g_ids.push_back(id);
      g_local_ids.push_back(i);
      g_local_counts.push_back(count);
      g_hosts.push_back(strlen(hosts[i]) ? hosts[i] : "127.0.0.1");
      g_arguments.push_back(std::vector<std::string>(argv, argv + argc));
    }

    return 0;
  }
  catch(std::exception& e)
  {
    std::cerr << "PHISH BAIT ERROR: " << e.what() << std::endl;
    return -1;
  }
}

int phish_bait_hook(const char* output_id, int output_port, const char* style, int input_port, const char* input_id)
{
  try
  {
    if(0 == g_id_map.count(output_id))
    {
      std::ostringstream message;
      message << "Unknown output id: " << output_id;
      throw std::runtime_error(message.str());
    }
    if(0 == g_id_map.count(input_id))
    {
      std::ostringstream message;
      message << "Unknown input id: " << input_id;
      throw std::runtime_error(message.str());
    }

    std::vector<int>& output_minnows = g_id_map[output_id];
    std::vector<int>& input_minnows = g_id_map[input_id];

    if(std::string(style) == PHISH_BAIT_SINGLE)
    {
      if(input_minnows.size() != 1)
        throw std::runtime_error("Single hook style requires one input minnow.");

      for(int i = 0; i != output_minnows.size(); ++i)
      {
        g_connections.push_back(connection(output_minnows[i], output_port, "broadcast", input_port, input_minnows));
      }
    }
    else if(std::string(style) == PHISH_BAIT_PAIRED)
    {
      if(output_minnows.size() != input_minnows.size())
        throw std::runtime_error("Paired hook style requires equal numbers of output and input minnows.");

      for(int i = 0; i != output_minnows.size(); ++i)
      {
        g_connections.push_back(connection(output_minnows[i], output_port, "broadcast", input_port, std::vector<int>(1, input_minnows[i])));
      }
    }
    else if(std::string(style) == PHISH_BAIT_HASHED)
    {
      for(int i = 0; i != output_minnows.size(); ++i)
      {
        g_connections.push_back(connection(output_minnows[i], output_port, "hashed", input_port, input_minnows));
      }
    }
    else if(std::string(style) == PHISH_BAIT_ROUND_ROBIN)
    {
      for(int i = 0; i != output_minnows.size(); ++i)
      {
        g_connections.push_back(connection(output_minnows[i], output_port, "round-robin", input_port, input_minnows));
      }
    }
    else if(std::string(style) == PHISH_BAIT_DIRECT)
    {
      for(int i = 0; i != output_minnows.size(); ++i)
      {
        g_connections.push_back(connection(output_minnows[i], output_port, "direct", input_port, input_minnows));
      }
    }
    else if(std::string(style) == PHISH_BAIT_BROADCAST)
    {
      for(int i = 0; i != output_minnows.size(); ++i)
      {
        g_connections.push_back(connection(output_minnows[i], output_port, "broadcast", input_port, input_minnows));
      }
    }
    else if(std::string(style) == PHISH_BAIT_CHAIN)
    {
      if(std::string(output_id) != std::string(input_id))
        throw std::runtime_error("Chain hook style requires the same id for output and input minnows.");

      for(int i = 0; (i+1) < output_minnows.size(); ++i)
      {
        g_connections.push_back(connection(output_minnows[i], output_port, "broadcast", input_port, std::vector<int>(1, input_minnows[(i+1)])));
      }
    }
    else if(std::string(style) == PHISH_BAIT_RING)
    {
      if(std::string(output_id) != std::string(input_id))
        throw std::runtime_error("Ring hook style requires the same id for output and input minnows.");

      for(int i = 0; i < output_minnows.size(); ++i)
      {
        g_connections.push_back(connection(output_minnows[i], output_port, "broadcast", input_port, std::vector<int>(1, input_minnows[(i+1) % input_minnows.size()])));
      }
    }
    else if(std::string(style) == PHISH_BAIT_PUBLISH)
    {
    }
    else if(std::string(style) == PHISH_BAIT_SUBSCRIBE)
    {
    }
    else
    {
      throw std::runtime_error("Unknown hook style");
    }

    return 0;
  }
  catch(std::exception& e)
  {
    std::cerr << "PHISH BAIT ERROR: " << e.what() << std::endl;
    return -1;
  }
}

/*
void phish_bait_all_to_all(int output_count, const int* output_minnows, int output_port, const char* send_pattern, int input_port, int input_count, const int* input_minnows)
{
  for(int i = 0; i != output_count; ++i)
  {
    g_connections.push_back(connection(output_minnows[i], output_port, send_pattern, input_port, std::vector<int>(input_minnows, input_minnows + input_count)));
  }
}

int phish_bait_one_to_one(int output_count, const int* output_minnows, int output_port, int input_port, int input_count, const int* input_minnows)
{
  try
  {
    if(output_count != input_count)
      throw std::runtime_error("Input and output minnow counts must match.");

    for(int i = 0; i != output_count; ++i)
    {
      g_connections.push_back(connection(output_minnows[i], output_port, "broadcast", input_port, std::vector<int>(1, input_minnows[i])));
    }

    return 0;
  }
  catch(std::exception& e)
  {
    std::cerr << "PHISH BAIT ERROR: " << e.what() << std::endl;
    return -1;
  }
}

int phish_bait_loop(int output_count, const int* output_minnows, int output_port, int input_port, int input_count, const int* input_minnows)
{
  try
  {
    if(output_count != input_count)
      throw std::runtime_error("Input and output minnow counts must match.");

    for(int i = 0; i != output_count; ++i)
    {
      g_connections.push_back(connection(output_minnows[i], output_port, "broadcast", input_port, std::vector<int>(1, input_minnows[(i + 1) % output_count])));
    }

    return 0;
  }
  catch(std::exception& e)
  {
    std::cerr << "PHISH BAIT ERROR: " << e.what() << std::endl;
    return -1;
  }
}
*/
