#!/usr/local/bin/python

# MINNOW sort
# sort datums, emit top N

import phish
import sys

def store(nvalues):
  if nvalues != 2: phish.error("Sort processes two-value datums")
  type, count, temp = phish.unpack()
  type, key, temp = phish.unpack()
  hash[key] = count

def sort():
  for key, value in sorted(hash.items(), key=lambda x: x[1],
                           reverse=True)[:ntop]:
    phish.pack_int32(value)
    phish.pack_string(key)
    phish.send(0)

argv = phish.init(sys.argv)
phish.input(0, store, sort, 1)
phish.output(0)
phish.check()

if len(argv) != 2: phish.error("Sort syntax: python sort.py N")
ntop = int(argv[1])

hash = {}

phish.loop()
phish.exit()

