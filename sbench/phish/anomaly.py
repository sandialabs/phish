#!/usr/local/bin/python

# MINNOW anomaly
# print found anomalies to screen

import sys
import phish

kv = {}

anomaly = 0
correct = 0
pfalse = 0
nfalse = 0

# thresh = N,M
# flag as anomaly if key appears N times with value=1 <= M times
# thresh = 12,2 flags many false positives

nthresh = 24
mthresh = 4

def frame(nvalues):
  global anomaly,correct,pfalse,nfalse
  type,str,len = phish.unpack()
  lines = str.split('\n')
    
  for line in lines[1:-1]:
    words = line.split(',')

    # store datum in hash table
    # value = (N,M)
    # N = # of times key has been seen
    # M = # of times flag = 1
    # M < 0 means already seen N times
      
    key = int(words[0])
    flag = int(words[1])

    if key not in kv: kv[key] = (1,flag)
    else:
      n,m = kv[key]
      if m < 0:
        kv[key] = (n+1,m)
        continue
      n += 1
      m += flag
      if n >= nthresh:
        truth = int(words[2])
        if m > mthresh:
          if truth:
            nfalse += 1
            print "FALSE NEG",key,truth
        else:
          anomaly += 1
          if truth:
            correct += 1
            print "ANOMALY",key,truth
          else:
            pfalse += 1
            print "FALSE POS",key,truth
        m = -1
      kv[key] = (n,m)

args = phish.init(sys.argv)
phish.input(0,frame,None,1)
phish.check()

phish.loop()
phish.exit()
