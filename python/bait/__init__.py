import ctypes
import ctypes.util

_library = None

SINGLE = "single"
PAIRED = "paired"
HASHED = "hashed"
ROUND_ROBIN = "round-robin"
DIRECT = "direct"
BROADCAST = "broadcast"
CHAIN = "chain"
RING = "ring"
PUBLISH = "publish"
SUBSCRIBE = "subscribe"

def reset():
  _library.phish_bait_reset()

def minnows(id, hosts, arguments):
  count = len(hosts)
  argc = len(arguments)
  _library.phish_bait_minnows(
    ctypes.c_char_p(id),
    ctypes.c_int(count),
    ctypes.byref((ctypes.c_char_p * count)(*hosts)),
    ctypes.c_int(argc),
    ctypes.byref((ctypes.c_char_p * argc)(*arguments)),
    )

def hook(output_id, output_port, style, input_port, input_id):
  _library.phish_bait_all_to_all(
    ctypes.c_char_p(output_id),
    ctypes.c_int(output_port),
    ctypes.c_char_p(style),
    ctypes.c_int(input_port),
    ctypes.c_char_p(input_id)
    )

def start():
  _library.phish_bait_start()
