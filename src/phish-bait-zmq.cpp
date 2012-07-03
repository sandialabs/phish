#include <phish-bait.h>

#include <zmq.hpp>

#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <unistd.h>
#include <sys/errno.h>
#include <sys/wait.h>

template<typename T>
std::string string_cast(const T& value)
{
  std::ostringstream buffer;
  buffer << value;
  return buffer.str();
}

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

struct out_connection
{
  out_connection(const std::string& sp, int ip, const std::vector<std::string>& ia) :
    send_pattern(sp),
    input_port(ip),
    input_addresses(ia)
  {
  }

  std::string send_pattern;
  int input_port;
  std::vector<std::string> input_addresses;
};

static std::vector<std::string> g_names;
static std::vector<int> g_local_ids;
static std::vector<int> g_local_counts;
static std::vector<std::string> g_hosts;
static std::vector<std::vector<std::string> > g_arguments;

static std::vector<connection> g_connections;

void phish_bait_reset()
{
  g_names.clear();
  g_local_ids.clear();
  g_local_counts.clear();
  g_hosts.clear();
  g_arguments.clear();
  g_connections.clear();
}

void phish_bait_add_minnows(const char* name, int count, const char** hosts, int argc, const char** argv, int* minnows)
{
/*
  std::cerr << name << std::endl;
  std::cerr << "  " << count << std::endl;
  for(int i = 0; i != count; ++i)
    std::cerr << "  " << hosts[i] << std::endl;
  std::cerr << "  " << argc << std::endl;
  for(int i = 0; i != argc; ++i)
    std::cerr << "  " << argv[i] << std::endl;
  for(int i = 0; i != count; ++i)
    std::cerr << "  " << minnows[i] << std::endl;
*/

  for(int i = 0; i != count; ++i)
  {
    minnows[i] = g_names.size();

    g_names.push_back(name);
    g_local_ids.push_back(i);
    g_local_counts.push_back(count);
    g_hosts.push_back(strlen(hosts[i]) ? hosts[i] : "127.0.0.1");
    g_arguments.push_back(std::vector<std::string>(argv, argv + argc));
  }
}

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

int phish_bait_start()
{
  try
  {
    int next_port = 5555;

    // Assign control port addresses ...
    std::vector<std::string> control_ports_internal;
    std::vector<std::string> control_ports_external;
    for(int i = 0; i != g_names.size(); ++i)
    {
      control_ports_internal.push_back("tcp://*:" + string_cast(next_port));
      control_ports_external.push_back("tcp://" + g_hosts[i] + ":" + string_cast(next_port));
      ++next_port;
    }

    // Assign input port addresses ...
    std::vector<std::string> input_ports_internal;
    std::vector<std::string> input_ports_external;
    for(int i = 0; i != g_names.size(); ++i)
    {
      input_ports_internal.push_back("tcp://*:" + string_cast(next_port));
      input_ports_external.push_back("tcp://" + g_hosts[i] + ":" + string_cast(next_port));
      ++next_port;
    }

    // Count input port incoming connections ...
    std::vector<std::map<int, int> > input_connection_counts(g_names.size(), std::map<int, int>());
    for(std::vector<connection>::iterator connection = g_connections.begin(); connection != g_connections.end(); ++connection)
    {
      for(std::vector<int>::iterator input_minnow = connection->input_minnows.begin(); input_minnow != connection->input_minnows.end(); ++input_minnow)
      {
        input_connection_counts[*input_minnow].insert(std::make_pair(connection->input_port, 0));
        input_connection_counts[*input_minnow][connection->input_port] += 1;
      }
    }

    // Keep track of output port outgoing connections ...
    std::vector<std::map<int, std::vector<out_connection> > > output_connections(g_names.size(), std::map<int, std::vector<out_connection> >());
    for(std::vector<connection>::iterator connection = g_connections.begin(); connection != g_connections.end(); ++connection)
    {
      std::vector<std::string> input_addresses;
      for(int i = 0; i != connection->input_minnows.size(); ++i)
        input_addresses.push_back(input_ports_external[connection->input_minnows[i]]);

      output_connections[connection->output_minnow].insert(std::make_pair(connection->output_port, std::vector<out_connection>()));
      output_connections[connection->output_minnow][connection->output_port].push_back(out_connection(connection->send_pattern, connection->input_port, input_addresses));
    }

    std::vector<pid_t> processes;

    // Create each of the minnow processes ...
    for(int i = 0; i != g_names.size(); ++i)
    {
      const std::string name = g_names[i];
      const int local_id = g_local_ids[i];
      const int global_id = i;
      const std::string host = g_hosts[i];

      std::vector<std::string> arguments;
      arguments.insert(arguments.end(), g_arguments[i].begin(), g_arguments[i].end());
      arguments.push_back("--phish-host");
      arguments.push_back(host);
      arguments.push_back("--phish-name");
      arguments.push_back(name);
      arguments.push_back("--phish-local-id");
      arguments.push_back(string_cast(local_id));
      arguments.push_back("--phish-local-count");
      arguments.push_back(string_cast(g_local_counts[i]));
      arguments.push_back("--phish-global-id");
      arguments.push_back(string_cast(global_id));
      arguments.push_back("--phish-global-count");
      arguments.push_back(string_cast(g_names.size()));
      arguments.push_back("--phish-control-port");
      arguments.push_back(control_ports_internal[i]);
      arguments.push_back("--phish-input-port");
      arguments.push_back(input_ports_internal[i]);

      for(std::map<int, int>::iterator j = input_connection_counts[i].begin(); j != input_connection_counts[i].end(); ++j)
      {
        const int port = j->first;
        const int count = j->second;

        std::ostringstream buffer;
        buffer << port << "+" << count;

        arguments.push_back("--phish-input-connections");
        arguments.push_back(buffer.str());
      }

      for(std::map<int, std::vector<out_connection> >::iterator j = output_connections[i].begin(); j != output_connections[i].end(); ++j)
      {
        const int output_port = j->first;
        for(std::vector<out_connection>::iterator connection = j->second.begin(); connection != j->second.end(); ++connection)
        {
          std::ostringstream buffer;
          for(int k = 0; k != connection->input_addresses.size(); ++k)
            buffer << "+" << connection->input_addresses[k];

          arguments.push_back("--phish-output-connection");
          arguments.push_back(string_cast(output_port) + "+" + connection->send_pattern + "+" + string_cast(connection->input_port) + buffer.str());
        }
      }

      if(host != "localhost" && host != "127.0.0.1")
      {
        arguments.insert(arguments.begin(), host);
        arguments.insert(arguments.begin(), "-x");
        arguments.insert(arguments.begin(), "ssh");
      }

      std::copy(arguments.begin(), arguments.end(), std::ostream_iterator<std::string>(std::cerr, " "));
      std::cerr << std::endl;

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
          processes.push_back(pid);
          continue;
        }
      }
    }

    // Tell each minnow to begin processing ...
    zmq::context_t context(2);
    for(int i = 0; i != processes.size(); ++i)
    {
      zmq::socket_t socket(context, ZMQ_REQ);
      socket.connect(control_ports_external[i].c_str());
      zmq::message_t request_message(const_cast<char*>("start"), strlen("start"), 0);
      socket.send(request_message, 0);

      zmq::message_t response_message;
      socket.recv(&response_message, 0);
      const std::string response(reinterpret_cast<char*>(response_message.data()), response_message.size());
      if(response != "ok")
        throw std::runtime_error("Minnow failed to start.");
    }

    // Wait for processes to terminate ...
    for(int i = 0; i != processes.size(); ++i)
    {
      int status = 0;
      int options = 0;
      ::waitpid(processes[i], &status, options);
    }
  }
  catch(std::exception& e)
  {
    std::cerr << "PHISH BAIT ERROR: " << e.what() << std::endl;
    return -1;
  }
}

