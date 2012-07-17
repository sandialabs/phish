/* ----------------------------------------------------------------------
   PHISH = Parallel Harness for Informatic Stream Hashing
   http://www.sandia.gov/~sjplimp/phish.html
   Steve Plimpton, sjplimp@sandia.gov, Sandia National Laboratories

   Copyright (2012) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under 
   the modified Berkeley Software Distribution (BSD) License.

   See the README file in the top-level PHISH directory.
------------------------------------------------------------------------- */

#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <hash.h>
#include <phish.h>
#include <phish-bait-common.h>
#include <phish-common.h>
#include <zmq.hpp>

#define phish_return_error(message, code) { phish_error(message); return code; }

///////////////////////////////////////////////////////////////////////////////////
// Internal state

// ZMQ context and incoming sockets ...
static zmq::context_t* g_context = 0;
static zmq::socket_t* g_control_port = 0;
static zmq::socket_t* g_input_port = 0;

// Input port configuration ...

// Stores a message callback for each input port ...
static std::map<int, void(*)(int)> g_input_port_message_callback;
// Stores a port-closed callback for each input port ...
static std::map<int, void(*)()> g_input_port_closed_callback;
// Stores whether an input port is optional ...
static std::map<int, bool> g_input_port_required;
// Stores the number of incoming connections for each input port ...
static std::map<int, int> g_input_port_connection_count;

// Temporary storage for packing outgoing messages ...
static std::vector<zmq::message_t*> g_pack_messages;

// Temporary storage for unpacking incoming messages ...
static std::vector<zmq::message_t*> g_unpack_messages;
static uint32_t g_unpack_count = 0;
static uint32_t g_unpack_index = 0;

// Keeps track of whether a message loop is running ...
static bool g_running = false;

// Keeps track of message statistics ...
static uint64_t g_received_count = 0;
static uint64_t g_sent_count = 0;
static uint64_t g_sent_close_count = 0;

/// Defines a collection of message recipients (zmq sockets).
typedef std::vector<zmq::socket_t*> recipients_t;
/// Abstract interface for classes that route messages to their destination(s).
class output_connection
{
public:
  output_connection(int input_port, const recipients_t& recipients);
  ~output_connection();
  virtual void send() = 0;
  virtual void send_hashed(char* key, int key_length) = 0;
  virtual void send_direct(int destination) = 0;

protected:
  const int m_input_port;
  const recipients_t m_recipients;
  void raw_send(zmq::socket_t& recipient);
};

static std::map<int, std::vector<output_connection*> > g_output_connections;
static std::set<int> g_defined_output_ports;

//////////////////////////////////////////////////////////////////////////////////
// Internal implementation details

static inline void pack(uint8_t type, uint32_t count, uint32_t size, const void* data)
{
  zmq::message_t* const message = new zmq::message_t(sizeof(type) + sizeof(count) + (count * size));
  uint8_t* message_data = reinterpret_cast<uint8_t*>(message->data());

  *reinterpret_cast<uint8_t*>(message_data) = type;
  message_data += sizeof(uint8_t);
  *reinterpret_cast<uint32_t*>(message_data) = count;
  message_data += sizeof(uint32_t);

  ::memcpy(message_data, data, count * size);

  g_pack_messages.push_back(message);
}

template<typename T>
static inline void pack_value(const T& value, uint8_t type)
{
  pack(type, 1, sizeof(T), &value);
}

template<typename T>
static inline void pack_array(const T* array, int32_t count, uint8_t type)
{
  pack(type, count, sizeof(T), array);
}

/// Defines a container for a collection of ports.
typedef std::vector<int> port_collection;
const port_collection output_ports()
{
  port_collection ports;
  for(std::map<int, std::vector<output_connection*> >::iterator i = g_output_connections.begin(); i != g_output_connections.end(); ++i)
    ports.push_back(i->first);
  return ports;
}

/// Defines constants for managing message framing & control.
enum message_frame
{
  PORT_MASK = 0x7f,
  TYPE_MASK = 0x80,
  CLOSE_MESSAGE = 0x80
};

void zmq_assert(int rc)
{
  if(rc < 0)
    throw std::runtime_error(zmq_strerror(errno));
}

// output_connection implementation.
output_connection::output_connection(int input_port, const recipients_t& recipients) :
  m_input_port(input_port),
  m_recipients(recipients)
{
}

output_connection::~output_connection()
{
  for(recipients_t::const_iterator recipient = m_recipients.begin(); recipient != m_recipients.end(); ++recipient)
  {
    zmq::message_t frame(1);
    reinterpret_cast<uint8_t*>(frame.data())[0] = ((m_input_port & PORT_MASK) | CLOSE_MESSAGE);
    zmq_assert((*recipient)->send(frame));
    ++g_sent_close_count;
  }

/* Don't close the zmq sockets, see https://zeromq.jira.com/browse/LIBZMQ-229

  for(recipients_t::const_iterator recipient = m_recipients.begin(); recipient != m_recipients.end(); ++recipient)
  {
    delete *recipient;
  }
*/
}

void output_connection::raw_send(zmq::socket_t& recipient)
{
  zmq::message_t frame(1);
  reinterpret_cast<uint8_t*>(frame.data())[0] = (m_input_port & PORT_MASK);
  zmq_assert(recipient.send(frame, g_pack_messages.size() ? ZMQ_SNDMORE : 0));

  for(int i = 0; i != g_pack_messages.size(); ++i)
  {
    zmq::message_t message;
    message.copy(g_pack_messages[i]);
    zmq_assert(recipient.send(message, (i+1) != g_pack_messages.size() ? ZMQ_SNDMORE : 0));
  }
}

/// output_connection implementation that broadcasts messages to every recipient.
class broadcast_connection :
  public output_connection
{
public:
  broadcast_connection(int input_port, const recipients_t& recipients) :
    output_connection(input_port, recipients)
  {
  }

  void send()
  {
    for(recipients_t::const_iterator recipient = m_recipients.begin(); recipient != m_recipients.end(); ++recipient)
    {
      raw_send(**recipient);
    }
  }

  void send_hashed(char* key, int key_length)
  {
    phish_warn("Cannot send hashed with broadcast connection.");
  }

  void send_direct(int destination)
  {
    phish_warn("Cannot send direct with broadcast connection.");
  }
};

/// output_connection implementation that sends a message to one recipient, choosing recipients in round-robin order.
class round_robin_connection :
  public output_connection
{
  int m_index;

public:
  round_robin_connection(int input_port, const recipients_t& recipients) :
    output_connection(input_port, recipients),
    m_index(0)
  {
  }

  void send()
  {
    zmq::socket_t* const recipient = m_recipients[m_index];
    m_index = (m_index + 1) % m_recipients.size();
    raw_send(*recipient);
  }

  void send_hashed(char* key, int key_length)
  {
    phish_warn("Cannot send hashed with roundrobin connection.");
  }

  void send_direct(int destination)
  {
    phish_warn("Cannot send direct with roundrobin connection.");
  }
};

/// output_connection implementation that sends a message to one recipient, choosing recipients using a hashed key supplied by the caller.
class hashed_connection :
  public output_connection
{
public:
  hashed_connection(int input_port, const recipients_t& recipients) :
    output_connection(input_port, recipients)
  {
  }

  void send()
  {
    phish_warn("Cannot send over hashed connection without key.");
  }

  void send_hashed(char* key, int key_length)
  {
    int index = hashlittle(key, key_length, 0) % m_recipients.size();
    zmq::socket_t* const recipient = m_recipients[index];
    raw_send(*recipient);
  }

  void send_direct(int destination)
  {
    phish_warn("Cannot send direct with hashed connection.");
  }
};

/// output_connection implementation that sends a two-part message to one recipient, which is specified by the caller.
class direct_connection :
  public output_connection
{
public:
  direct_connection(int input_port, const recipients_t& recipients) :
    output_connection(input_port, recipients)
  {
  }

  void send()
  {
    phish_warn("Cannot send over direct connection without recipient.");
  }

  void send_hashed(char* key, int key_length)
  {
    phish_warn("Cannot send hashed with direct connection.");
  }

  void send_direct(int destination)
  {
    int index = destination % m_recipients.size();
    zmq::socket_t* const recipient = m_recipients[index];
    raw_send(*recipient);
  }
};

///////////////////////////////////////////////////////////////////////////////////
// Public API

// Compatibility API for minnows written in C ...
extern "C"
{

int phish_init(int* argc, char*** argv)
{
  phish_assert_not_initialized();

  std::vector<std::string> arguments(*argv, *argv + *argc);
  std::vector<std::string> kept_arguments;

  g_executable = arguments[0];

  g_context = new zmq::context_t(2);
  while(arguments.size())
  {
    const std::string argument = pop_argument(arguments);
    if(argument == "--phish-host")
    {
      g_host = pop_argument(arguments);
    }
    else if(argument == "--phish-school-id")
    {
      g_school_id = pop_argument(arguments);
    }
    else if(argument == "--phish-local-id")
    {
      std::istringstream stream(pop_argument(arguments));
      stream >> g_local_id;
    }
    else if(argument == "--phish-local-count")
    {
      std::istringstream stream(pop_argument(arguments));
      stream >> g_local_count;
    }
    else if(argument == "--phish-global-id")
    {
      std::istringstream stream(pop_argument(arguments));
      stream >> g_global_id;
    }
    else if(argument == "--phish-global-count")
    {
      std::istringstream stream(pop_argument(arguments));
      stream >> g_global_count;
    }
    else if(argument == "--phish-control-port")
    {
      const std::string address = pop_argument(arguments);
      g_control_port = new zmq::socket_t(*g_context, ZMQ_REP);
      g_control_port->bind(address.c_str());
    }
    else if(argument == "--phish-input-port")
    {
      const std::string address = pop_argument(arguments);
      g_input_port = new zmq::socket_t(*g_context, ZMQ_PULL);
      g_input_port->bind(address.c_str());
    }
    else if(argument == "--phish-input-connections")
    {
      std::string spec = pop_argument(arguments);
      std::replace(spec.begin(), spec.end(), '+', ' ');
      int port = 0;
      int connection_count = 0;
      std::istringstream stream(spec);
      stream >> port >> connection_count;

      g_input_port_connection_count[port] = connection_count;

    }
    else if(argument == "--phish-output-connection")
    {
      std::string spec = pop_argument(arguments);
      std::replace(spec.begin(), spec.end(), '+', ' ');
      int output_port = 0;
      std::string pattern;
      int input_port = 0;
      std::vector<std::string> recipients;
      std::istringstream stream(spec);
      stream >> output_port >> pattern >> input_port;
      while(true)
      {
        std::string recipient;
        stream >> recipient;
        if(!stream)
          break;
        recipients.push_back(recipient);
      }

      if(!g_output_connections.count(output_port))
        g_output_connections[output_port] = std::vector<output_connection*>();

      recipients_t recipient_sockets;
      for(std::vector<std::string>::iterator recipient = recipients.begin(); recipient != recipients.end(); ++recipient)
      {
        zmq::socket_t* const socket = new zmq::socket_t(*g_context, ZMQ_PUSH);
        const uint64_t hwm = 100000;
        socket->setsockopt(ZMQ_HWM, &hwm, sizeof(hwm));
        socket->connect(recipient->c_str());
        recipient_sockets.push_back(socket);
      }

      if(pattern == PHISH_BAIT_SEND_PATTERN_BROADCAST)
      {
        g_output_connections[output_port].push_back(new broadcast_connection(input_port, recipient_sockets));
      }
      else if(pattern == PHISH_BAIT_SEND_PATTERN_ROUND_ROBIN)
      {
        g_output_connections[output_port].push_back(new round_robin_connection(input_port, recipient_sockets));
      }
      else if(pattern == PHISH_BAIT_SEND_PATTERN_HASHED)
      {
        g_output_connections[output_port].push_back(new hashed_connection(input_port, recipient_sockets));
      }
      else if(pattern == PHISH_BAIT_SEND_PATTERN_DIRECT)
      {
        g_output_connections[output_port].push_back(new direct_connection(input_port, recipient_sockets));
      }
      else
      {
        std::ostringstream message;
        message << "Unknown send pattern: " << pattern;
        phish_return_error(message.str().c_str(), -1);
      }
    }
    else
    {
      kept_arguments.push_back(argument);
    }
  }

  // Wait to hear from the school ...
  zmq::message_t message;
  g_control_port->recv(&message, 0);
  const std::string request(reinterpret_cast<char*>(message.data()), message.size());
  if(request == "start")
  {
    zmq::message_t message(const_cast<char*>("ok"), strlen("ok"), 0);
    g_control_port->send(message, 0);
  }
  else
  {
    std::ostringstream message;
    message << "Unexpected request: " << request;
    phish_return_error(message.str().c_str(), -1);
  }

  // Cleanup argc & argv ...
  *argc = get_argc(kept_arguments);
  *argv = get_argv(kept_arguments);

  return 0;
}

int phish_exit()
{
  phish_assert_initialized();
  phish_assert_checked();

  // Close output ports ...
  const port_collection ports = output_ports();
  for(port_collection::const_iterator port = ports.begin(); port != ports.end(); ++port)
    phish_close(*port);

  // Cancel any running loop ...
  g_running = false;

  // Delete input port ...
  delete g_input_port;
  g_input_port = 0;

  // Delete control port ...
  delete g_control_port;
  g_control_port = 0;

  std::ostringstream message;
  message << g_received_count << " " << g_sent_count << " datums recv/sent";
  phish_message("Stats", message.str().c_str());

  // Shut-down zmq ...
/* Don't try to shutdown zmq, see https://zeromq.jira.com/browse/LIBZMQ-229
  delete g_context;
*/
  ::sleep(5);
  g_context = 0;

  return 0;
}

void phish_abort()
{
  if(!phish_abort_internal())
    return;

  phish_warn("Currently, phish_abort() doesn't shut-down the entire school.");
  exit(-1);
}

int phish_input(int port, void(*message_callback)(int), void(*port_closed_callback)(), int required)
{
  phish_assert_initialized();
  phish_assert_not_checked();

  g_input_port_message_callback[port] = message_callback;
  g_input_port_closed_callback[port] = port_closed_callback;
  g_input_port_required[port] = required;

  return 0;
}

int phish_output(int port)
{
  phish_assert_initialized();
  phish_assert_not_checked();

  g_defined_output_ports.insert(port);

  return 0;
}

int phish_check()
{
  phish_assert_initialized();
  phish_assert_not_checked();

  for(std::map<int, int>::iterator port = g_input_port_connection_count.begin(); port != g_input_port_connection_count.end(); ++port)
  {
    if(!g_input_port_message_callback.count(port->first))
    {
      std::ostringstream message;
      message << g_school_id << ": unexpected connection to undefined input port " << port->first;
      phish_return_error(message.str().c_str(), -1);
    }
  }
  for(std::map<int, void(*)(int)>::iterator port = g_input_port_message_callback.begin(); port != g_input_port_message_callback.end(); ++port)
  {
    if(!g_input_port_connection_count.count(port->first) && g_input_port_required[port->first])
    {
      std::ostringstream message;
      message << g_school_id << ": required input port " << port->first << " does not have a connection.";
      phish_return_error(message.str().c_str(), -1);
    }
  }
  for(std::map<int, std::vector<output_connection*> >::iterator port = g_output_connections.begin(); port != g_output_connections.end(); ++port)
  {
    if(!g_defined_output_ports.count(port->first))
    {
      std::ostringstream message;
      message << g_school_id << ": unexpected connection from undefined output port " << port->first;
      phish_return_error(message.str().c_str(), -1);
    }
  }

  return 0;
}

int phish_close(int port)
{
  phish_assert_initialized();
  phish_assert_checked();

  if(!g_output_connections.count(port))
    return 0;
  for(int i = 0; i != g_output_connections[port].size(); ++i)
    delete g_output_connections[port][i];
  g_output_connections.erase(port);

  return 0;
}

int phish_loop()
{
  phish_assert_initialized();
  phish_assert_checked();

  if(g_running)
  {
    phish_warn("Cannot call phish_loop() while a loop is already running.");
    return 0;
  }
  g_running = true;

  while(g_running)
  {
    try
    {
      zmq::message_t frame_message;
      zmq_assert(g_input_port->recv(&frame_message, 0));
      const uint8_t frame = reinterpret_cast<uint8_t*>(frame_message.data())[0];
      const int port = (frame & PORT_MASK);

      if((frame & TYPE_MASK) == CLOSE_MESSAGE)
      {
        g_input_port_connection_count[port] -= 1;
        if(g_input_port_connection_count[port] == 0)
        {
          g_input_port_connection_count.erase(port);
          if(g_input_port_closed_callback.count(port) && g_input_port_closed_callback[port])
          {
            g_input_port_closed_callback[port]();
          }
          if(g_input_port_connection_count.size() == 0)
          {
            g_running = false;
            if(g_all_input_ports_closed)
              g_all_input_ports_closed();
          }
        }
      }
      else
      {
        g_received_count += 1;

        g_unpack_index = 0;
        g_unpack_count = 0;
        int64_t more_parts = 0;
        size_t more_parts_size = sizeof(more_parts);
        for(g_input_port->getsockopt(ZMQ_RCVMORE, &more_parts, &more_parts_size); more_parts; g_input_port->getsockopt(ZMQ_RCVMORE, &more_parts, &more_parts_size))
        {
          while(g_unpack_messages.size() <= g_unpack_count)
            g_unpack_messages.push_back(new zmq::message_t());
          zmq_assert(g_input_port->recv(g_unpack_messages[g_unpack_count], 0));
          ++g_unpack_count;
        }
        g_input_port_message_callback[port](g_unpack_count);
      }
    }
    catch(std::exception& e)
    {
      phish_warn(e.what());
    }
  }

  return 0;
}

int phish_probe(void (*idle_callback)())
{
  phish_assert_initialized();
  phish_assert_checked();

  if(g_running)
  {
    phish_warn("Cannot call phish_probe() while a loop is already running.");
    return 0;
  }
  g_running = true;

  while(g_running)
  {
    try
    {
      zmq::message_t frame_message;
      const int rc = g_input_port->recv(&frame_message, ZMQ_NOBLOCK);
      if(0 == rc)
      {
        const uint8_t frame = reinterpret_cast<uint8_t*>(frame_message.data())[0];
        const int port = (frame & PORT_MASK);

        if((frame & TYPE_MASK) == CLOSE_MESSAGE)
        {
          g_input_port_connection_count[port] -= 1;
          if(g_input_port_connection_count[port] == 0)
          {
            g_input_port_connection_count.erase(port);
            if(g_input_port_closed_callback.count(port))
            {
              g_input_port_closed_callback[port]();
            }
            if(g_input_port_connection_count.size() == 0)
            {
              g_running = false;
              if(g_all_input_ports_closed)
                g_all_input_ports_closed();
            }
          }
        }
        else
        {
          g_received_count += 1;

          g_unpack_index = 0;
          g_unpack_count = 0;
          int64_t more_parts = 0;
          size_t more_parts_size = sizeof(more_parts);
          for(g_input_port->getsockopt(ZMQ_RCVMORE, &more_parts, &more_parts_size); more_parts; g_input_port->getsockopt(ZMQ_RCVMORE, &more_parts, &more_parts_size))
          {
            while(g_unpack_messages.size() <= g_unpack_count)
              g_unpack_messages.push_back(new zmq::message_t());
            zmq_assert(g_input_port->recv(g_unpack_messages[g_unpack_count], 0));
            ++g_unpack_count;
          }
          g_input_port_message_callback[port](g_unpack_count);
        }
      }
      else if(errno == EAGAIN)
      {
        idle_callback();
      }
      else
      {
        zmq_assert(rc);
      }
    }
    catch(std::exception& e)
    {
      phish_warn(e.what());
    }
  }

  return 0;
}

int phish_recv()
{
  if(g_running)
  {
    phish_warn("Cannot call phish_recv() while a loop is running.");
    return -1;
  }

  try
  {
    zmq::message_t frame_message;
    const int rc = g_input_port->recv(&frame_message, ZMQ_NOBLOCK);
    if(0 == rc)
    {
      const uint8_t frame = reinterpret_cast<uint8_t*>(frame_message.data())[0];
      const int port = (frame & PORT_MASK);

      if((frame & TYPE_MASK) == CLOSE_MESSAGE)
      {
        g_input_port_connection_count[port] -= 1;
        if(g_input_port_connection_count[port] == 0)
        {
          g_input_port_connection_count.erase(port);
          if(g_input_port_closed_callback.count(port))
          {
            g_input_port_closed_callback[port]();
          }
          if(g_input_port_connection_count.size() == 0)
          {
            g_running = false;
            if(g_all_input_ports_closed)
              g_all_input_ports_closed();
          }
        }
        return -1;
      }
      else
      {
        g_received_count += 1;

        g_unpack_index = 0;
        g_unpack_count = 0;
        int64_t more_parts = 0;
        size_t more_parts_size = sizeof(more_parts);
        for(g_input_port->getsockopt(ZMQ_RCVMORE, &more_parts, &more_parts_size); more_parts; g_input_port->getsockopt(ZMQ_RCVMORE, &more_parts, &more_parts_size))
        {
          while(g_unpack_messages.size() <= g_unpack_count)
            g_unpack_messages.push_back(new zmq::message_t());
          zmq_assert(g_input_port->recv(g_unpack_messages[g_unpack_count], 0));
          ++g_unpack_count;
        }
        return g_unpack_count;
      }
    }
    else if(errno == EAGAIN)
    {
      return 0;
    }
    else
    {
      zmq_assert(rc);
    }
  }
  catch(std::exception& e)
  {
    phish_warn(e.what());
    return -1;
  }
}

void phish_send(int port)
{
  if(!g_output_connections.count(port))
  {
    std::ostringstream message;
    message << "Cannot send message to closed port: " << port;
    phish_warn(message.str().c_str());
    return;
  }

  const int end = g_output_connections[port].size();
  for(int i = 0; i != end; ++i)
    g_output_connections[port][i]->send();

  g_sent_count += 1;

  for(int i = 0; i != g_pack_messages.size(); ++i)
    delete g_pack_messages[i];
  g_pack_messages.resize(0);
}

void phish_send_key(int port, char* key, int key_length)
{
  if(!g_output_connections.count(port))
  {
    std::ostringstream message;
    message << "Cannot send message to closed port: " << port;
    phish_warn(message.str().c_str());
    return;
  }

  const int end = g_output_connections[port].size();
  for(int i = 0; i != end; ++i)
    g_output_connections[port][i]->send_hashed(key, key_length);

  g_sent_count += 1;

  for(int i = 0; i != g_pack_messages.size(); ++i)
    delete g_pack_messages[i];
  g_pack_messages.resize(0);
}

void phish_send_direct(int port, int receiver)
{
  if(!g_output_connections.count(port))
  {
    std::ostringstream message;
    message << "Cannot send message to closed port: " << port;
    phish_warn(message.str().c_str());
    return;
  }

  const int end = g_output_connections[port].size();
  for(int i = 0; i != end; ++i)
    g_output_connections[port][i]->send_direct(receiver);

  g_sent_count += 1;

  for(int i = 0; i != g_pack_messages.size(); ++i)
    delete g_pack_messages[i];
  g_pack_messages.resize(0);
}

void phish_reset_receiver(int, int)
{
  phish_warn("phish_reset_receiver() Not implemented.");
}

void phish_repack()
{
  for(uint32_t i = 0; i != g_unpack_count; ++i)
  {
    zmq::message_t* const source = g_unpack_messages[i];
    zmq::message_t* const destination = new zmq::message_t(source->size());
    ::memcpy(destination->data(), source->data(), source->size());
    g_pack_messages.push_back(destination);
  }
}

void phish_pack_raw(char* data, int32_t length)
{
  pack(PHISH_RAW, length, sizeof(char), data);
}

void phish_pack_char(char value)
{
  pack_value(value, PHISH_CHAR);
}

void phish_pack_int8(int8_t value)
{
  pack_value(value, PHISH_INT8);
}

void phish_pack_int16(int16_t value)
{
  pack_value(value, PHISH_INT16);
}

void phish_pack_int32(int32_t value)
{
  pack_value(value, PHISH_INT32);
}

void phish_pack_int64(int64_t value)
{
  pack_value(value, PHISH_INT64);
}

void phish_pack_uint8(uint8_t value)
{
  pack_value(value, PHISH_UINT8);
}

void phish_pack_uint16(uint16_t value)
{
  pack_value(value, PHISH_UINT16);
}

void phish_pack_uint32(uint32_t value)
{
  pack_value(value, PHISH_UINT32);
}

void phish_pack_uint64(uint64_t value)
{
  pack_value(value, PHISH_UINT64);
}

void phish_pack_float(float value)
{
  pack_value(value, PHISH_FLOAT);
}

void phish_pack_double(double value)
{
  pack_value(value, PHISH_DOUBLE);
}

void phish_pack_string(char* value)
{
  pack(PHISH_STRING, strlen(value) + 1, sizeof(char), value);
}

void phish_pack_int8_array(int8_t* array, int32_t count)
{
  pack_array(array, count, PHISH_INT8_ARRAY);
}

void phish_pack_int16_array(int16_t* array, int32_t count)
{
  pack_array(array, count, PHISH_INT16_ARRAY);
}

void phish_pack_int32_array(int32_t* array, int32_t count)
{
  pack_array(array, count, PHISH_INT32_ARRAY);
}

void phish_pack_int64_array(int64_t* array, int32_t count)
{
  pack_array(array, count, PHISH_INT64_ARRAY);
}

void phish_pack_uint8_array(uint8_t* array, int32_t count)
{
  pack_array(array, count, PHISH_UINT8_ARRAY);
}

void phish_pack_uint16_array(uint16_t* array, int32_t count)
{
  pack_array(array, count, PHISH_UINT16_ARRAY);
}

void phish_pack_uint32_array(uint32_t* array, int32_t count)
{
  pack_array(array, count, PHISH_UINT32_ARRAY);
}

void phish_pack_uint64_array(uint64_t* array, int32_t count)
{
  pack_array(array, count, PHISH_UINT64_ARRAY);
}

void phish_pack_float_array(float* array, int32_t count)
{
  pack_array(array, count, PHISH_FLOAT_ARRAY);
}

void phish_pack_double_array(double* array, int32_t count)
{
  pack_array(array, count, PHISH_DOUBLE_ARRAY);
}

void phish_pack_pickle(char* data, int32_t count)
{
  pack(PHISH_PICKLE, count, sizeof(char), data);
}

int phish_unpack(char** data, int32_t* count)
{
  if(g_unpack_index >= g_unpack_count)
    phish_return_error("No data to unpack.", -1);

  const uint8_t* message_data = reinterpret_cast<uint8_t*>(g_unpack_messages[g_unpack_index]->data());

  uint8_t type = *reinterpret_cast<const uint8_t*>(message_data);
  message_data += sizeof(uint8_t);

  *count = *reinterpret_cast<const uint32_t*>(message_data);
  message_data += sizeof(uint32_t);

  *data = const_cast<char*>(reinterpret_cast<const char*>(message_data));

  g_unpack_index += 1;
  return type;
}

int phish_query(const char* kw, int flag1, int flag2)
{
  const std::string keyword(kw);

  if(keyword == "idlocal")
    return g_local_id;
  else if(keyword == "nlocal")
    return g_local_count;
  else if(keyword == "idglobal")
    return g_global_id;
  else if(keyword == "nglobal")
    return g_global_count;
  else
    phish_return_error("Invalid phish_query keyword.", -1);
}

} // extern "C"
