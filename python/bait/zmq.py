import ctypes
import sys

if sys.modules["bait"]._library is None:
  if ctypes.util.find_library("phish-bait-zmq"):
    sys.modules["bait"]._library = ctypes.CDLL(ctypes.util.find_library("phish-bait-zmq"))
if sys.modules["bait"]._library is None:
  sys.modules["bait"]._library = ctypes.CDLL("libphish-bait-zmq.so")
if sys.modules["bait"]._library is None:
  sys.modules["bait"]._library = ctypes.CDLL("libphish-bait-zmq.dylib")
if sys.modules["bait"]._library is None:
  sys.modules["bait"]._library = ctypes.CDLL("libphish-bait-zmq.dll")
if sys.modules["bait"]._library is None:
  raise Exception("Unable to locate PHISH bait-zmq library.")

