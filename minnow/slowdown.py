#!/usr/local/bin/python

# MINNOW slowdown
# read datum and emit it with slowdown delay

import sys,time
import phish

def send(nvalues):
  global time_previous,delta
  elapsed = phish.timer() - time_previous
  if elapsed < delta: time.sleep(delta-elapsed)
  phish.repack()
  phish.send(0)
  time_previous = phish.timer()

args = phish.init(sys.argv)
phish.input(0,send,None,1)
phish.output(0)
phish.check()

if len(args) != 2: phish.error("Slowdown syntax: slowdown delta")

delta = float(args[1])
time_previous = phish.timer()

phish.loop()
phish.exit()
