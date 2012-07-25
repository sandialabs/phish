#!/usr/local/bin/python

import phish
import sys

def store(nvalues):
  if nvalues != 2:
    phish.error("sort.py processes two-value datums.")

  type, count, temp = phish.unpack()
  type, key, temp = phish.unpack()
  hash[key] = count

def sort():
  for key, value in sorted(hash.items(), key=lambda x: x[1], reverse=True)[:n]:
    phish.pack_int32(value)
    phish.pack_string(key)
    phish.send(0)

argv = phish.init(sys.argv)

if len(argv) != 2:
  phish.error("sort.py syntax: python sort.py N")
n = int(argv[1])

phish.input(0, store, sort, 1)
phish.output(0)
phish.check()

hash = {}

phish.loop()
phish.exit()

