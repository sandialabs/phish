#!/usr/local/bin/python

# MINNOW count
# count instances of strings

import sys
import phish

def count(nvalues):
  if nvalues != 1: phish.error("Count processes one-value datums")
  type,str,tmp = phish.unpack()
  if type != phish.STRING:
    phish.error("Count processes string values")
  if hash.has_key(str): hash[str] = hash[str] + 1
  else: hash[str] = 1

def dump():
  for key,value in hash.items():
    phish.pack_int32(value)
    phish.pack_string(key)
    phish.send(0)

argv = phish.init(sys.argv)
phish.input(0,count,dump,1)
phish.output(0)
phish.check()

if len(argv) != 1: phish.error("Count syntax: count");

hash = {}

phish.loop()
phish.exit()
