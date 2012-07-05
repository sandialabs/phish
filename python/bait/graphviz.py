import ctypes
import sys

if sys.modules["bait"]._library is None:
  if ctypes.util.find_library("phish-bait-graphviz"):
    sys.modules["bait"]._library = ctypes.CDLL(ctypes.util.find_library("phish-bait-graphviz"))
if sys.modules["bait"]._library is None:
  sys.modules["bait"]._library = ctypes.CDLL("libphish-bait-graphviz.so")
if sys.modules["bait"]._library is None:
  sys.modules["bait"]._library = ctypes.CDLL("libphish-bait-graphviz.dylib")
if sys.modules["bait"]._library is None:
  sys.modules["bait"]._library = ctypes.CDLL("libphish-bait-graphviz.dll")
if sys.modules["bait"]._library is None:
  raise Exception("Unable to locate PHISH bait-graphviz library.")

