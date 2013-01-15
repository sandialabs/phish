#!/usr/local/python

# scanfile.py = scan file of archived UDP output
# Syntax: scanfile.py < infile

import sys

f = sys.stdin

count = 0
nframe = 0
imin = 1000000000000
imax = -1

n = 0
while 1:
  str = f.readline()
  if not str: break
  if "frame" in str:
    nframe += 1
    continue
  n += 1
  words = str.split(',')
  if int(words[2]): count += 1
  imin = min(imin,int(words[0]))
  imax = max(imax,int(words[0]))

print
print "%d frames" % nframe
print "%d lines" % n
print "%d hits" % count
print "%d %d min/max" % (imin,imax)
