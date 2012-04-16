import ctypes
import ctypes.util

_library = None
if _library is None:
  if ctypes.util.find_library("phish-school-zmq"):
    _library = ctypes.CDLL(ctypes.util.find_library("phish-school-zmq"))
if _library is None:
  _library = ctypes.CDLL("libphish-school-zmq.so")
if _library is None:
  _library = ctypes.CDLL("libphish-school-zmq.dylib")
if _library is None:
  _library = ctypes.CDLL("libphish-school-zmq.dll")
if _library is None:
  raise Exception("Unable to locate PHISH SCHOOL library.")

ROUND_ROBIN = "round-robin"
HASHED = "hashed"
BROADCAST = "broadcast"
DIRECT = "direct"

def reset():
  _library.phish_school_reset()

def add_minnows(name, hosts, arguments):
  count = len(hosts)
  argc = len(arguments)
  minnows = (ctypes.c_int * count)()
  _library.phish_school_add_minnows(
    ctypes.c_char_p(name),
    ctypes.c_int(count),
    ctypes.byref((ctypes.c_char_p * count)(*hosts)),
    ctypes.c_int(argc),
    ctypes.byref((ctypes.c_char_p * argc)(*arguments)),
    ctypes.byref(minnows)
    )
  return [minnow for minnow in minnows]

def all_to_all(output_minnows, output_port, send_pattern, input_port, input_minnows):
  output_count = len(output_minnows)
  input_count = len(input_minnows)
  _library.phish_school_all_to_all(
    ctypes.c_int(output_count),
    ctypes.byref((ctypes.c_int * output_count)(*output_minnows)),
    ctypes.c_int(output_port),
    ctypes.c_char_p(send_pattern),
    ctypes.c_int(input_port),
    ctypes.c_int(input_count),
    ctypes.byref((ctypes.c_int * input_count)(*input_minnows))
    )

def one_to_one(output_minnows, output_port, input_port, input_minnows):
  output_count = len(output_minnows)
  input_count = len(input_minnows)
  _library.phish_school_one_to_one(
    ctypes.c_int(output_count),
    ctypes.byref((ctypes.c_int * output_count)(*output_minnows)),
    ctypes.c_int(output_port),
    ctypes.c_int(input_port),
    ctypes.c_int(input_count),
    ctypes.byref((ctypes.c_int * input_count)(*input_minnows))
    )

def start():
  _library.phish_school_start()
