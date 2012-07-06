#include <phish-bait.h>
#include <phish-bait-common.h>

#include <zmq.hpp>

#include <iostream>
#include <iterator>
#include <map>
#include <stdexcept>

struct zmq_minnow
{
  std::string control_port_internal;
  std::string control_port_external;
  std::string input_port_internal;
  std::string input_port_external;
};

int phish_bait_start()
{
  try
  {
    int next_port = 5555;

    // Setup temporary storage for zmq-specific information
    std::vector<zmq_minnow> zmq_minnows(g_minnows.size());

    // Assign control port addresses ...
    for(int i = 0; i != zmq_minnows.size(); ++i)
    {
      const school& school = g_schools[g_minnows[i].school_index];
      const minnow& minnow = g_minnows[i];

      zmq_minnows[i].control_port_internal = "tcp://*:" + string_cast(next_port);
      zmq_minnows[i].control_port_external = "tcp://" + school.hosts[minnow.local_id] + ":" + string_cast(next_port);
      ++next_port;
    }

    // Assign input port addresses ...
    for(int i = 0; i != zmq_minnows.size(); ++i)
    {
      const school& school = g_schools[g_minnows[i].school_index];
      const minnow& minnow = g_minnows[i];

      zmq_minnows[i].input_port_internal = "tcp://*:" + string_cast(next_port);
      zmq_minnows[i].input_port_external = "tcp://" + school.hosts[minnow.local_id] + ":" + string_cast(next_port);
      ++next_port;
    }

    std::vector<pid_t> processes;

    // Create each of the minnow processes ...
    for(int i = 0; i != zmq_minnows.size(); ++i)
    {
      const school& school = g_schools[g_minnows[i].school_index];
      const minnow& minnow = g_minnows[i];
      const std::string host = school.hosts[minnow.local_id];

      std::vector<std::string> arguments;
      arguments.insert(arguments.end(), school.arguments.begin(), school.arguments.end());
      arguments.push_back("--phish-host");
      arguments.push_back(host);
      arguments.push_back("--phish-name");
      arguments.push_back(school.id);
      arguments.push_back("--phish-local-id");
      arguments.push_back(string_cast(minnow.local_id));
      arguments.push_back("--phish-local-count");
      arguments.push_back(string_cast(school.hosts.size()));
      arguments.push_back("--phish-global-id");
      arguments.push_back(string_cast(i));
      arguments.push_back("--phish-global-count");
      arguments.push_back(string_cast(g_minnows.size()));
      arguments.push_back("--phish-control-port");
      arguments.push_back(zmq_minnows[i].control_port_internal);
      arguments.push_back("--phish-input-port");
      arguments.push_back(zmq_minnows[i].input_port_internal);

      for(std::map<int, int>::const_iterator incoming = minnow.incoming.begin(); incoming != minnow.incoming.end(); ++incoming)
      {
        const int port = incoming->first;
        const int count = incoming->second;

        std::ostringstream buffer;
        buffer << port << "+" << count;

        arguments.push_back("--phish-input-connections");
        arguments.push_back(buffer.str());
      }

      for(std::vector<connection>::const_iterator connection = minnow.outgoing.begin(); connection != minnow.outgoing.end(); ++connection)
      {
        std::ostringstream buffer;
        for(std::vector<int>::const_iterator i = connection->input_indices.begin(); i != connection->input_indices.end(); ++i)
          buffer << "+" << zmq_minnows[*i].input_port_external;

        arguments.push_back("--phish-output-connection");
        arguments.push_back(string_cast(connection->output_port) + "+" + connection->send_pattern + "+" + string_cast(connection->input_port) + buffer.str());
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
      socket.connect(zmq_minnows[i].control_port_external.c_str());
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

