#ifndef PHISH_HPP
#define PHISH_HPP

#include <iostream>
#include <string>
#include <tr1/cstdint>
#include <tr1/functional>
#include <vector>

namespace phish
{

/// Defines a zero-based unique identifier for minnows within a school.
typedef uint32_t rank_id;
/// Defines a zero-based unique identifier for minnow input and output ports.
typedef uint8_t port_index;
/// Defines a container for a collection of ports.
typedef std::vector<port_index> port_collection;
/// Defines a callback to receive incoming messages.
typedef std::tr1::function<void(int32_t)> message_callback;
/// Defines a callback to be called when an input port is closed.
typedef std::tr1::function<void()> port_closed_callback;
/// Defines a callback to be called when the last input port is closed.
typedef std::tr1::function<void()> last_port_closed_callback;

enum data_type
{
  RAW = 0,
  UINT8 = 1,
  UINT32 = 2,
  UINT64 = 3,
  FLOAT32 = 4,
  FLOAT64 = 5,
  STRING = 6
};

/// Initializes a minnow and prepares for communication with the rest of the
/// school.  This should be called as soon as possible after minnow startup, and
/// may modify the supplied argv.  Throws std::runtime_error if there are any problems
/// initializing the minnow or establishing communication with the rest of the school.
void init(int& argc, char**& argv);

bool debug();
void log_debug(const std::string& message);

/// Returns the human-readable name for this minnow.
const std::string name();

/// Returns the rank (a zero-based ID) for this minnow.
const rank_id rank();

/// Returns a list of connected input ports.  Minnows may use this information
/// to configure varying numbers of inputs at runtime.  Note that this function
/// will return different results as ports are closed over the lifetime of the minnow.
const port_collection input_ports();

/// Returns a list of connected output ports.  Minnows may use this information
/// to configure varying numbers of outputs at runtime.  Note that this function
/// will return different results as ports are closed over the lifetime of the minnow.
const port_collection output_ports();

/// Specifies an input port to be enabled, along with an optional callback to
/// be called when a message is received on the port, an optional callback to be
/// called when the port is closed (i.e. all connections to the port are closed),
/// and a flag specifying whether the port is optional (i.e. whether it is an
/// error condition if there are no connections to this port).
void input(port_index port, message_callback message, port_closed_callback port_closed, bool optional=false);

/// Specifies an output port to be enabled.  It is an error to attempt
/// sending a message on a port that hasn't been enabled, or to make a
/// connection to such a port.
void output(port_index port);

/// Compares the set of connections to-and-from this minnow to its configured
/// input and output ports to detect setup errors.  Throws std::runtime_error
/// if there are any problems.
void check();

/// Specifies a callback to be called exactly once when every input port has
/// been closed.  Note: this callback will never be called for minnows that
/// have no input ports.
void set_last_port_closed_callback(last_port_closed_callback last_port_closed);

/// Called to begin an event loop that will receive messages and invoke
/// registered callbacks.  Loops cannot be nested, so subsequent calls to
/// loop() before calling loop_complete() will be ignored.
void loop();

/// Called to exit an event loop started with loop().  Note that multiple
/// calls to loop_complete() and calls to loop_complete() before calling loop()
/// will be ignored.
void loop_complete();

void pack(uint8_t data);
void pack(uint32_t data);
void pack(uint64_t data);
void pack(float data);
void pack(double data);
void pack(const char* data);
void pack(const std::string& data);

data_type unpack_type();
uint32_t unpack_length();
void skip_part();

void unpack(uint8_t& data);
void unpack(uint32_t& data);
void unpack(uint64_t& data);
void unpack(float& data);
void unpack(double& data);
void unpack(std::string& data);

template<typename T>
const T unpack()
{
  T value;
  unpack(value);
  return value;
}

/// Sends a message from the given output port.  The message recipients will
/// depend on the type(s) of connections to the port (round-robin, broadcast,
/// hashed, etc.).  Note that sending a message to a nonexistent / closed port
/// will throw std::runtime_error.
void send(port_index output_port=0);

/// Notifies downstream minnows that the given output port has been closed.
void close_port(port_index output_port);

/// Shuts-down phish and releases any resources allocated by the library.
/// Note that this also implicitly closes any open output ports and completes
/// any running loop.
void close();

namespace timer
{

class cpu
{
public:
  cpu();
  double elapsed() const;
  void reset();

private:
  double start;
};

class wallclock
{
public:
  wallclock();
  double elapsed() const;
  void reset();

private:
  double start;
};

} // namespace timer

} // namespace phish

#endif // !PHISH_HPP
