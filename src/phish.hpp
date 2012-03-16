#ifndef PHISH_HPP
#define PHISH_HPP

#include <string>
#include <stdexcept>
#include <vector>

#include <phish.h>

namespace phish
{

enum data_type
{
  RAW = PHISH_RAW,
  CHAR = PHISH_CHAR,
  INT8 = PHISH_INT8,
  INT16 = PHISH_INT16,
  INT32 = PHISH_INT32,
  INT64 = PHISH_INT64,
  UINT8 = PHISH_UINT8,
  UINT16 = PHISH_UINT16,
  UINT32 = PHISH_UINT32,
  UINT64 = PHISH_UINT64,
  FLOAT = PHISH_FLOAT,
  DOUBLE = PHISH_DOUBLE,
  STRING = PHISH_STRING,
  INT8_ARRAY = PHISH_INT8_ARRAY,
  INT16_ARRAY = PHISH_INT16_ARRAY,
  INT32_ARRAY = PHISH_INT32_ARRAY,
  INT64_ARRAY = PHISH_INT64_ARRAY,
  UINT8_ARRAY = PHISH_UINT8_ARRAY,
  UINT16_ARRAY = PHISH_UINT16_ARRAY,
  UINT32_ARRAY = PHISH_UINT32_ARRAY,
  UINT64_ARRAY = PHISH_UINT64_ARRAY,
  FLOAT_ARRAY = PHISH_FLOAT_ARRAY,
  DOUBLE_ARRAY = PHISH_DOUBLE_ARRAY,
  PICKLE = PHISH_PICKLE,
};

//#define LOG_CALL() { ::phish_warn(__PRETTY_FUNCTION__); }
#define LOG_CALL() {}

/// Initializes a minnow and prepares for communication with the rest of the
/// school.  This should be called as soon as possible after minnow startup, and
/// may modify the supplied argv.  Throws std::runtime_error if there are any problems
/// initializing the minnow or establishing communication with the rest of the school.
void init(int& argc, char**& argv) { LOG_CALL(); ::phish_init(&argc, &argv); }
/// Shuts-down phish and releases any resources allocated by the library.
/// Note that this also implicitly closes any open output ports and completes
/// any running loop.
void exit() { LOG_CALL(); ::phish_exit(); }
void atabort(voidcallback(*)(int*)) { LOG_CALL(); ::phish_atabort(callback); }
void abort() { LOG_CALL(); ::phish_abort(); }

/// Specifies an input port to be enabled, along with an optional callback to
/// be called when a message is received on the port, an optional callback to be
/// called when the port is closed (i.e. all connections to the port are closed),
/// and a flag specifying whether the port is optional (i.e. whether it is an
/// error condition if there are no connections to this port).
void input(int port, void (*message)(int), void (*port_closed)(), bool optional=false) { LOG_CALL(); ::phish_input(port, message, port_closed, optional); }
/// Specifies an output port to be enabled.  It is an error to attempt
/// sending a message on a port that hasn't been enabled, or to make a
/// connection to such a port.
void output(int port) { LOG_CALL(); ::phish_output(port); }
/// Compares the set of connections to-and-from this minnow to its configured
/// input and output ports to detect setup errors.  Throws std::runtime_error
/// if there are any problems.
void check() { LOG_CALL(); ::phish_check(); }
/// Specifies a callback to be called exactly once when every input port has
/// been closed.  Note: this callback will never be called for minnows that
/// have no input ports.
void done(void (*callback)()) { LOG_CALL(); ::phish_done(callback); }
/// Notifies downstream minnows that the given output port has been closed.
void close(int port) { LOG_CALL(); ::phish_close(port); }


/// Called to begin an event loop that will receive messages and invoke
/// registered callbacks.  Loops cannot be nested, so subsequent calls to
/// loop() will be ignored.
void loop() { LOG_CALL(); ::phish_loop(); }

/// Sends a message from the given output port.  The message recipients will
/// depend on the type(s) of connections to the port (round-robin, broadcast,
/// hashed, etc.).  Note that sending a message to a nonexistent / closed port
/// will throw std::runtime_error.
void send(int port = 0) { LOG_CALL(); ::phish_send(port); }
void send_key(const char* key, int key_length, int port = 0) { LOG_CALL(); ::phish_send_key(port, const_cast<char*>(key), key_length); }

void pack(char data) { LOG_CALL(); ::phish_pack_char(data); }
void pack(int8_t data) { LOG_CALL(); ::phish_pack_int8(data); }
void pack(int16_t data) { LOG_CALL(); ::phish_pack_int16(data); }
void pack(int32_t data) { LOG_CALL(); ::phish_pack_int32(data); }
void pack(int64_t data) { LOG_CALL(); ::phish_pack_int64(data); }
void pack(uint8_t data) { LOG_CALL(); ::phish_pack_uint8(data); }
void pack(uint16_t data) { LOG_CALL(); ::phish_pack_uint16(data); }
void pack(uint32_t data) { LOG_CALL(); ::phish_pack_uint32(data); }
void pack(uint64_t data) { LOG_CALL(); ::phish_pack_uint64(data); }
void pack(float data) { LOG_CALL(); ::phish_pack_float(data); }
void pack(double data) { LOG_CALL(); ::phish_pack_double(data); }
void pack(const char* data) { LOG_CALL(); ::phish_pack_string(const_cast<char*>(data)); }
void pack(const std::string& data) { LOG_CALL(); ::phish_pack_string(const_cast<char*>(data.c_str())); }

data_type unpack(char*& data, uint32_t& count) { return static_cast<data_type>(::phish_unpack(&data, &count)); }

template<typename T>
void unpack(T& data, data_type type)
{
  char* buffer;
  uint32_t count;
  if(unpack(buffer, count) != type)
    throw std::runtime_error("Data type mismatch.");
  data = *reinterpret_cast<T*>(buffer);
}

void unpack(char& data) { LOG_CALL(); unpack(data, CHAR); }
void unpack(int8_t& data) { LOG_CALL(); unpack(data, INT8); }
void unpack(int16_t& data) { LOG_CALL(); unpack(data, INT16); }
void unpack(int32_t& data) { LOG_CALL(); unpack(data, INT32); }
void unpack(int64_t& data) { LOG_CALL(); unpack(data, INT64); }
void unpack(uint8_t& data) { LOG_CALL(); unpack(data, UINT8); }
void unpack(uint16_t& data) { LOG_CALL(); unpack(data, UINT16); }
void unpack(uint32_t& data) { LOG_CALL(); unpack(data, UINT32); }
void unpack(uint64_t& data) { LOG_CALL(); unpack(data, UINT64); }
void unpack(float& data) { LOG_CALL(); unpack(data, FLOAT); }
void unpack(double& data) { LOG_CALL(); unpack(data, DOUBLE); }
void unpack(std::string& data)
{
  LOG_CALL();
  char* buffer;
  uint32_t length;
  if(unpack(buffer, length) != STRING)
    throw std::runtime_error("Data type mismatch.");
  data = std::string(buffer, length-1);
}

template<typename T>
const T unpack()
{
  T value;
  unpack(value);
  return value;
}

void error(const std::string& message) { ::phish_error(message.c_str()); }
void warn(const std::string& message) { ::phish_warn(message.c_str()); }
void debug(const std::string& message) { ::phish_warn(message.c_str()); }
double timer() { return ::phish_timer(); }

} // namespace phish

#endif // !PHISH_HPP
