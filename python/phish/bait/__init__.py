import ctypes
import ctypes.util

_library = None

SINGLE = "single"
PAIRED = "paired"
HASHED = "hashed"
ROUND_ROBIN = "roundrobin"
DIRECT = "direct"
BROADCAST = "broadcast"
CHAIN = "chain"
RING = "ring"
PUBLISH = "publish"
SUBSCRIBE = "subscribe"

def backend(name):
  global _library
  _library = None
  if _library is None:
    if ctypes.util.find_library("phish-bait-%s" % name):
      _library = ctypes.CDLL(ctypes.util.find_library("phish-bait-%s" % name))
  if _library is None:
    _library = ctypes.CDLL("libphish-bait-%s.so" % name)
  if _library is None:
    _library = ctypes.CDLL("libphish-bait-%s.dylib" % name)
  if _library is None:
    _library = ctypes.CDLL("libphish-bait-%s.dll" % name)
  if _library is None:
    raise Exception("Unable to load %s backend." % name)

def reset():
  _library.phish_bait_reset()

def set(name, value):
  _library.phish_bait_set(
    ctypes.c_char_p(name),
    ctypes.c_char_p(value)
    )

def school(id, hosts, bind, arguments):
  count = len(hosts)
  if bind:
    bindptr = (ctypes.c_int*(2*count))()
    i = 0
    for pair in bind:
      bindptr[i] = pair[0]
      bindptr[i+1] = pair[1]
      i += 2
  else: ptr = None
  argc = len(arguments)
  # NOTE: could now add bindptr as arg to phish_bait_school()
  
  _library.phish_bait_school(
    ctypes.c_char_p(id),
    ctypes.c_int(count),
    ctypes.byref((ctypes.c_char_p * count)(*hosts)), # bindptr,
    ctypes.c_int(argc),
    ctypes.byref((ctypes.c_char_p * argc)(*arguments)),
    )

def hook(output_id, output_port, style, input_port, input_id):
  _library.phish_bait_hook(
    ctypes.c_char_p(output_id),
    ctypes.c_int(output_port),
    ctypes.c_char_p(style),
    ctypes.c_int(input_port),
    ctypes.c_char_p(input_id)
    )

def start():
  _library.phish_bait_start()
