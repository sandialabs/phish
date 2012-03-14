#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>

#include <hash.h>
#include <phish.hpp>
#include <zmq.hpp>

#include <sys/time.h>

///////////////////////////////////////////////////////////////////////////////////
// Internal implementation details

static uint64_t g_received_count = 0;
static uint64_t g_sent_count = 0;
static uint64_t g_sent_close_count = 0;

/// Defines a container for a collection of ports.
typedef std::vector<int> port_collection;
/// Defines a collection of message recipients (zmq sockets).
typedef std::vector<zmq::socket_t*> recipients_t;

/// Defines constants for managing message framing & control.
enum message_frame
{
  PORT_MASK = 0x7f,
  TYPE_MASK = 0x80,
  CLOSE_MESSAGE = 0x80
};

// Provides temporary storage for packing messages until they're handed-off to zmq ...
static std::vector<zmq::message_t*> g_pack_messages;

void zmq_assert(int rc)
{
  if(rc < 0)
    throw std::runtime_error(zmq_strerror(errno));
}

/// Abstract interface for classes that route messages to their destination(s).
class output_connection
{
protected:
  const int m_input_port;
  const recipients_t m_recipients;

  void raw_send(zmq::socket_t& recipient)
  {
    zmq::message_t frame(1);
    reinterpret_cast<uint8_t*>(frame.data())[0] = (m_input_port & PORT_MASK);
    zmq_assert(recipient.send(frame, g_pack_messages.size() ? ZMQ_SNDMORE : 0));

    for(int i = 0; i != g_pack_messages.size(); ++i)
    {
      zmq::message_t message;
      message.copy(g_pack_messages[i]);

/*
      std::cerr << "sending: ";
      std::copy(reinterpret_cast<uint8_t*>(message.data()), reinterpret_cast<uint8_t*>(message.data()) + message.size(), std::ostream_iterator<int>(std::cerr, " "));
      std::cerr << std::endl;
*/

      zmq_assert(recipient.send(message, (i+1) != g_pack_messages.size() ? ZMQ_SNDMORE : 0));
    }
  }

public:
  output_connection(int input_port, const recipients_t& recipients) :
    m_input_port(input_port),
    m_recipients(recipients)
  {
  }

  ~output_connection()
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

  virtual void send() = 0;
};

static zmq::context_t* g_context = 0;
static std::string g_name = std::string();
static int g_rank = 0;
static zmq::socket_t* g_control_port = 0;
static zmq::socket_t* g_input_port = 0;

static std::map<int, int> g_input_connection_counts;

static std::map<int, std::vector<output_connection*> > g_output_connections;

static std::map<int, phish::message_callback> g_message_callbacks;
static std::map<int, phish::port_closed_callback> g_closed_callbacks;
static std::map<int, bool> g_optional;

static std::set<int> g_defined_output_ports;

static phish::last_port_closed_callback g_last_port_closed;
static bool g_running = false;

// Provides temporary storage for unpacking messages ...
static std::vector<zmq::message_t*> g_unpack_messages;
static uint32_t g_unpack_count = 0;
static uint32_t g_unpack_index = 0;

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
};

/// output_connection implementation that sends a two-part message to one recipient, based on a hash of the first part.
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
    int index = hashlittle(reinterpret_cast<uint8_t*>(g_pack_messages[0]->data()) + 1, g_pack_messages[0]->size() - 1, 0) % m_recipients.size();
    zmq::socket_t* const recipient = m_recipients[index];
    raw_send(*recipient);
  }
};

static const std::string pop_argument(std::vector<std::string>& arguments)
{
  const std::string argument = arguments.front();
  arguments.erase(arguments.begin());
  return argument;
}

///////////////////////////////////////////////////////////////////////////////////
// Public API

namespace phish
{

const port_collection output_ports()
{
  port_collection ports;
  for(std::map<int, std::vector<output_connection*> >::iterator i = g_output_connections.begin(); i != g_output_connections.end(); ++i)
    ports.push_back(i->first);
  return ports;
}

void input(int port, message_callback message, port_closed_callback port_closed, bool optional)
{
  g_message_callbacks[port] = message;
  g_closed_callbacks[port] = port_closed;
  g_optional[port] = optional;
}

void set_last_port_closed_callback(last_port_closed_callback last_port_closed)
{
  g_last_port_closed = last_port_closed;
}

void loop()
{
  if(g_running)
    return;

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
        phish_warn("Received close message.");
        g_input_connection_counts[port] -= 1;
        if(g_input_connection_counts[port] == 0)
        {
          g_input_connection_counts.erase(port);
          if(g_closed_callbacks.count(port))
          {
            g_closed_callbacks[port]();
          }
          if(g_input_connection_counts.size() == 0 && g_last_port_closed)
          {
            g_last_port_closed();
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
        g_message_callbacks[port](g_unpack_count);
      }
    }
    catch(std::exception& e)
    {
      std::cerr << g_name << " (" << g_rank << ") " << e.what() << std::endl;
      //phish_warn(e.what());
    }
  }
}

void loop_complete()
{
  if(!g_running)
    return;
  g_running = false;
}

void send(int port)
{
  if(!g_output_connections.count(port))
  {
    std::ostringstream message;
    message << "Cannot send message to closed port: " << port;
    throw std::runtime_error(message.str());
  }

  const int end = g_output_connections[port].size();
  for(int i = 0; i != end; ++i)
    g_output_connections[port][i]->send();

  g_sent_count += 1;

  for(int i = 0; i != g_pack_messages.size(); ++i)
    delete g_pack_messages[i];
  g_pack_messages.resize(0);
}

data_type unpack_type()
{
  if(g_unpack_index >= g_unpack_count)
    throw std::runtime_error("No datum to type.");
  return static_cast<data_type>(reinterpret_cast<uint8_t*>(g_unpack_messages[g_unpack_index]->data())[0]);
}

uint32_t unpack_length()
{
  if(g_unpack_index >= g_unpack_count)
    throw std::runtime_error("No datum to size.");
  return g_unpack_messages[g_unpack_index]->size();
}

void skip_part()
{
  if(g_unpack_index >= g_unpack_count)
    throw std::runtime_error("No datum to skip.");
  g_unpack_index += 1;
}

template<typename T>
void unpack_helper(T& data, data_type type)
{
  if(g_unpack_index >= g_unpack_count)
    throw std::runtime_error("No datum to unpack.");
  if(reinterpret_cast<uint8_t*>(g_unpack_messages[g_unpack_index]->data())[0] != type)
    throw std::runtime_error("Data type mismatch.");
  if(g_unpack_messages[g_unpack_index]->size() != sizeof(T) + 1)
    throw std::runtime_error("Data size mismatch.");
  T temp; // This is a workaround for problems trying to take the address of data
  ::memcpy(&temp, reinterpret_cast<uint8_t*>(g_unpack_messages[g_unpack_index]->data()) + 1, sizeof(T));
  data = temp;
  g_unpack_index += 1;
}

void unpack(int8_t& data)
{
  unpack_helper(data, INT8);
}

void unpack(int16_t& data)
{
  unpack_helper(data, INT8);
}

void unpack(int32_t& data)
{
  unpack_helper(data, INT32);
}

void unpack(int64_t& data)
{
  unpack_helper(data, INT64);
}

void unpack(uint8_t& data)
{
  unpack_helper(data, UINT8);
}

void unpack(uint16_t& data)
{
  unpack_helper(data, UINT8);
}

void unpack(uint32_t& data)
{
  unpack_helper(data, UINT32);
}

void unpack(uint64_t& data)
{
  unpack_helper(data, UINT64);
}

void unpack(float& data)
{
  unpack_helper(data, FLOAT);
}

void unpack(double& data)
{
  unpack_helper(data, FLOAT);
}

void unpack(std::string& data)
{
  if(g_unpack_index >= g_unpack_count)
    throw std::runtime_error("No datum to unpack.");
  if(reinterpret_cast<uint8_t*>(g_unpack_messages[g_unpack_index]->data())[0] != STRING)
    throw std::runtime_error("Data type mismatch.");
  data.assign(reinterpret_cast<const char*>(g_unpack_messages[g_unpack_index]->data()) + 1, g_unpack_messages[g_unpack_index]->size() - 1);
  g_unpack_index += 1;
}

inline void pack_helper(const void* data, uint32_t length, int type)
{
  g_pack_messages.push_back(new zmq::message_t(1 + length));
  reinterpret_cast<uint8_t*>(g_pack_messages.back()->data())[0] = type;
  ::memcpy(reinterpret_cast<uint8_t*>(g_pack_messages.back()->data()) + 1, data, length);
}

template<typename T>
inline void pack_helper(const T& data, int type)
{
  pack_helper(&data, sizeof(T), type);
}

} // namespace phish

// Compatibility API for minnows written in C ...
extern "C"
{

void phish_init(int* argc, char*** argv)
{
  std::vector<std::string> arguments(*argv, *argv + *argc);
  std::vector<std::string> kept_arguments;

  g_context = new zmq::context_t(2);
  while(arguments.size())
  {
    const std::string argument = pop_argument(arguments);
    if(argument == "--phish-debug")
    {
    }
    else if(argument == "--phish-backend")
    {
      const std::string backend = pop_argument(arguments);
      //std::cerr << argument << " " << backend << std::endl;
      if(backend != "zmq")
      {
        std::ostringstream message;
        message << "Internal Phish library error: backend mismatch (got " << backend << ", expected zmq)";
        throw std::runtime_error(message.str());
      }
    }
    else if(argument == "--phish-name")
    {
      g_name = pop_argument(arguments);
      //std::cerr << argument << " " << g_name << std::endl;
    }
    else if(argument == "--phish-rank")
    {
      std::istringstream stream(pop_argument(arguments));
      stream >> g_rank;
      //std::cerr << argument << " " << g_rank << std::endl;
    }
    else if(argument == "--phish-control-port")
    {
      const std::string address = pop_argument(arguments);
      //std::cerr << argument << " " << address << std::endl;
      g_control_port = new zmq::socket_t(*g_context, ZMQ_REP);
      g_control_port->bind(address.c_str());
    }
    else if(argument == "--phish-input-port")
    {
      const std::string address = pop_argument(arguments);
      //std::cerr << argument << " " << address << std::endl;
      g_input_port = new zmq::socket_t(*g_context, ZMQ_PULL);
      g_input_port->bind(address.c_str());
    }
    else if(argument == "--phish-input-connections")
    {
      std::string spec = pop_argument(arguments);
      //std::cerr << argument << " " << spec << std::endl;
      std::replace(spec.begin(), spec.end(), '+', ' ');
      int port = 0;
      int connection_count = 0;
      std::istringstream stream(spec);
      stream >> port >> connection_count;

      g_input_connection_counts[port] = connection_count;

    }
    else if(argument == "--phish-output-connection")
    {
      std::string spec = pop_argument(arguments);
      //std::cerr << argument << " " << spec << std::endl;
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
        const uint64_t hwm = 1000000;
        socket->setsockopt(ZMQ_HWM, &hwm, sizeof(hwm));
        socket->connect(recipient->c_str());
        recipient_sockets.push_back(socket);

      }

      if(pattern == "broadcast")
      {
        g_output_connections[output_port].push_back(new broadcast_connection(input_port, recipient_sockets));
      }
      else if(pattern == "round-robin")
      {
        g_output_connections[output_port].push_back(new round_robin_connection(input_port, recipient_sockets));
      }
      else if(pattern == "hashed")
      {
        g_output_connections[output_port].push_back(new hashed_connection(input_port, recipient_sockets));
      }
      else
      {
        std::ostringstream message;
        message << "Unknown send pattern: " << pattern;
        throw std::runtime_error(message.str());
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
    throw std::runtime_error(message.str());
  }

  // Remove phish-specific arguments from argc & argv ...
  *argc = kept_arguments.size();
  *argv = new char*[kept_arguments.size()];
  for(int i = 0; i != kept_arguments.size(); ++i)
  {
    *argv[i] = new char[kept_arguments[i].size() + 1];
    strcpy(*argv[i], kept_arguments[i].c_str());
  }
}

int phish_init_python(int, char **)
{
  throw std::runtime_error("Not implemented.");
}

void phish_exit()
{
  // Close output ports ...
  const port_collection ports = phish::output_ports();
  for(port_collection::const_iterator port = ports.begin(); port != ports.end(); ++port)
    phish_close(*port);

  phish::loop_complete();

  // Delete input port ...
  delete g_input_port;
  g_input_port = 0;

  // Delete control port ...
  delete g_control_port;
  g_control_port = 0;

  std::ostringstream message;
  message << "Received " << g_received_count << " messages, sent " << g_sent_count << " messages, sent " << g_sent_close_count << " close messages.";
  phish_warn(message.str().c_str());

  // Shut-down zmq ...
  delete g_context;
  g_context = 0;
}

void phish_input(int, void(*)(int), void(*)(), int)
{
  throw std::runtime_error("Not implemented.");
}

void phish_output(int port)
{
  g_defined_output_ports.insert(port);
}

void phish_check()
{
  for(std::map<int, int>::iterator port = g_input_connection_counts.begin(); port != g_input_connection_counts.end(); ++port)
  {
    if(!g_message_callbacks.count(port->first))
    {
      std::ostringstream message;
      message << g_name << ": unexpected connection to undefined input port " << port->first;
      throw std::runtime_error(message.str());
    }
  }
  for(std::map<int, phish::message_callback>::iterator port = g_message_callbacks.begin(); port != g_message_callbacks.end(); ++port)
  {
    if(!g_input_connection_counts.count(port->first) && !g_optional[port->first])
    {
      std::ostringstream message;
      message << g_name << ": required input port " << port->first << " does not have a connection.";
      throw std::runtime_error(message.str());
    }
  }
  for(std::map<int, std::vector<output_connection*> >::iterator port = g_output_connections.begin(); port != g_output_connections.end(); ++port)
  {
    if(!g_defined_output_ports.count(port->first))
    {
      std::ostringstream message;
      message << g_name << ": unexpected connection from undefined output port " << port->first;
      throw std::runtime_error(message.str());
    }
  }
}

void phish_done(void (*)())
{
  throw std::runtime_error("Not implemented.");
}

void phish_close(int port)
{
  if(!g_output_connections.count(port))
    return;
  for(int i = 0; i != g_output_connections[port].size(); ++i)
    delete g_output_connections[port][i];
  g_output_connections.erase(port);
}

void phish_loop()
{
  throw std::runtime_error("Not implemented.");
}

void phish_probe(void (*)())
{
  throw std::runtime_error("Not implemented.");
}

int phish_recv()
{
  throw std::runtime_error("Not implemented.");
}

void phish_send(int)
{
  throw std::runtime_error("Not implemented.");
}

void phish_send_key(int, char *, int)
{
  throw std::runtime_error("Not implemented.");
}

void phish_send_direct(int, int)
{
  throw std::runtime_error("Not implemented.");
}

void phish_reset_receiver(int, int)
{
  throw std::runtime_error("Not implemented.");
}

void phish_pack_datum(char *, int)
{
  throw std::runtime_error("Not implemented.");
}

void phish_pack_raw(char *, int)
{
  throw std::runtime_error("Not implemented.");
}

void phish_pack_int8(int8_t data)
{
  phish::pack_helper(data, PHISH_INT8);
}

void phish_pack_int16(int16_t data)
{
  phish::pack_helper(data, PHISH_INT16);
}

void phish_pack_int32(int32_t data)
{
  phish::pack_helper(data, PHISH_INT32);
}

void phish_pack_int64(int64_t data)
{
  phish::pack_helper(data, PHISH_INT64);
}

void phish_pack_uint8(uint8_t data)
{
  phish::pack_helper(data, PHISH_UINT8);
}

void phish_pack_uint16(uint16_t data)
{
  phish::pack_helper(data, PHISH_UINT16);
}

void phish_pack_uint32(uint32_t data)
{
  phish::pack_helper(data, PHISH_UINT32);
}

void phish_pack_uint64(uint64_t data)
{
  phish::pack_helper(data, PHISH_UINT64);
}

void phish_pack_float(float data)
{
  phish::pack_helper(data, PHISH_FLOAT);
}

void phish_pack_double(double data)
{
  phish::pack_helper(data, PHISH_DOUBLE);
}

void phish_pack_string(char* data)
{
  phish::pack_helper(data, strlen(data) + 1, PHISH_STRING);
}

void phish_pack_int8_array(int8_t *, int)
{
  throw std::runtime_error("Not implemented.");
}

void phish_pack_int16_array(int16_t *, int)
{
  throw std::runtime_error("Not implemented.");
}

void phish_pack_int32_array(int32_t *, int)
{
  throw std::runtime_error("Not implemented.");
}

void phish_pack_int64_array(int64_t *, int)
{
  throw std::runtime_error("Not implemented.");
}

void phish_pack_uint8_array(uint8_t *, int)
{
  throw std::runtime_error("Not implemented.");
}

void phish_pack_uint16_array(uint16_t *, int)
{
  throw std::runtime_error("Not implemented.");
}

void phish_pack_uint32_array(uint32_t *, int)
{
  throw std::runtime_error("Not implemented.");
}

void phish_pack_uint64_array(uint64_t *, int)
{
  throw std::runtime_error("Not implemented.");
}

void phish_pack_float_array(float *, int)
{
  throw std::runtime_error("Not implemented.");
}

void phish_pack_double_array(double *, int)
{
  throw std::runtime_error("Not implemented.");
}

void phish_pack_pickle(char *, int)
{
  throw std::runtime_error("Not implemented.");
}

int phish_unpack(char **, int *)
{
  throw std::runtime_error("Not implemented.");
}

int phish_datum(char **, int *)
{
  throw std::runtime_error("Not implemented.");
}

int phish_query(const char *, int, int)
{
  throw std::runtime_error("Not implemented.");
}

void phish_error(const char* message)
{
  std::cerr << g_name << " (" << g_rank << ") - " << message << std::endl;
  throw std::runtime_error("Not implemented.");
}

void phish_warn(const char* message)
{
  std::cerr << g_name << " (" << g_rank << ") - " << message << std::endl;
}

double phish_timer()
{
  timeval t;
  ::gettimeofday(&t, 0);
  return t.tv_sec + (t.tv_usec / 1000000.0);
}

}
// extern "C"
