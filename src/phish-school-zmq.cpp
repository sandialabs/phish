#include <phish-school.h>

#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <unistd.h>

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

struct process
{
  virtual void wait() = 0;
  virtual void kill() = 0;
};

struct local_process :
  public process
{
  local_process(const std::string& name, int local_id, int global_id, const std::vector<std::string>& arguments) :
    pid(::fork())
  {
    if(0 == pid)
    {
      std::vector<char*> argv;
      for(int i = 0; i != arguments.size(); ++i)
        argv.push_back(const_cast<char*>(arguments[i].c_str()));
      argv.push_back(0);

      ::execv(arguments[0].c_str(), argv.data());
    }
    else
    {
    }
  }

  void wait()
  {
    int stat_loc = 0;
    int options = 0;
    ::waitpid(pid, &stat_loc, options);
  }

  void kill()
  {
  }

  const pid_t pid;
};

struct ssh_process :
  public process
{
  ssh_process(const std::string& name, int local_id, int global_id, const std::vector<std::string>& arguments, const std::string& host)
  {
  }

  void wait()
  {
  }

  void kill()
  {
  }
};

static std::vector<std::string> g_names;
static std::vector<int> g_local_ids;
static std::vector<int> g_local_counts;
static std::vector<std::string> g_hosts;
static std::vector<std::vector<std::string> > g_arguments;

static std::vector<connection> g_connections;

void phish_school_reset()
{
  g_names.clear();
  g_local_ids.clear();
  g_local_counts.clear();
  g_hosts.clear();
  g_arguments.clear();
  g_connections.clear();
}

void create_minnows(const char* name, int count, const char** hosts, int argc, const char** argv, int* minnows)
{
  for(int i = 0; i != count; ++i)
  {
    minnows[i] = g_names.size();

    g_names.push_back(name);
    g_local_ids.push_back(i);
    g_local_counts.push_back(count);
    g_hosts.push_back(hosts[i] ? hosts[i] : "127.0.0.1");
    g_arguments.push_back(std::vector<std::string>(argv, argv + argc));
  }
}

void all_to_all(int output_count, int* output_minnows, int output_port, const char* send_pattern, int input_port, int input_count, int* input_minnows)
{
  for(int i = 0; i != output_count; ++i)
  {
    g_connections.push_back(connection(output_minnows[i], output_port, send_pattern, input_port, std::vector<int>(input_minnows, input_minnows + input_count)));
  }
}

int one_to_one(int output_count, int* output_minnows, int output_port, int input_port, int input_count, int* input_minnows)
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
    std::cerr << "PHISH SCHOOL ERROR: " << e.what() << std::endl;
    return -1;
  }
}

int start()
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
    }

    // Assign input port addresses ...
    std::vector<std::string> input_ports_internal;
    std::vector<std::string> input_ports_external;
    for(int i = 0; i != g_names.size(); ++i)
    {
      input_ports_internal.push_back("tcp://*:" + string_cast(next_port));
      input_ports_external.push_back("tcp://" + g_hosts[i] + ":" + string_cast(next_port));
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
/*
  output_connections = [{} for name in _names]
  for output_minnow, output_port, send_pattern, input_port, input_minnows in _connections:
    if output_port not in output_connections[output_minnow]:
      output_connections[output_minnow][output_port] = []
    output_connections[output_minnow][output_port].append((send_pattern, input_port, [input_ports_external[input_minnow] for input_minnow in input_minnows]))
*/

    // Create minnows ...
    std::vector<process*> processes;
    for(int i = 0; i != g_names.size(); ++i)
    {
      const std::string name = g_names[i];
      const int local_id = g_local_ids[i];
      const int global_id = i;
      const std::string host = g_hosts[i];

      // Setup the command-line for the minnow ...
      std::vector<std::string> arguments;
      arguments.insert(arguments.end(), g_arguments[i].begin(), g_arguments[i].end());
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

/*
      for port, count in input_connection_counts[i].items():
        arguments += ["--phish-input-connections", "%s+%s" % (port, count)]
      for output_port, connections in output_connections[i].items():
        for send_pattern, input_port, minnows in connections:
          arguments += ["--phish-output-connection", "%s+%s+%s+%s" % (output_port, send_pattern, input_port, "+".join(minnows))]
*/

      if(host == "localhost" || host == "127.0.0.1")
        processes.push_back(new local_process(name, local_id, global_id, arguments));
      else
        processes.push_back(new ssh_process(name, local_id, global_id, arguments, host));
    }
/*
    # Tell each minnow to start processing ...
    for control_port in control_ports_external:
      socket = context.socket(zmq.REQ)
      socket.connect(control_port)
      socket.send("start")
      if socket.recv() != "ok":
        raise Exception("Minnow failed to start.")
*/

    // Wait for processes to terminate ...
    for(int i = 0; i != processes.size(); ++i)
      processes[i]->wait();


    for(int i = 0; i != processes.size(); ++i)
      delete processes[i];
  }
  catch(std::exception& e)
  {
    std::cerr << "PHISH SCHOOL ERROR: " << e.what() << std::endl;
    return -1;
  }
}

