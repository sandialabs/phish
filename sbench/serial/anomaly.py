#!/usr/local/python

# anomaly.py
# read frames from UDP socket, break into datums, look for anomalies
# Syntax: anomaly.py port

import sys,socket,re

IP = "127.0.0.1"

argv = sys.argv
if len(argv) != 2:
  print "Syntax: anomaly.py port"
  sys.exit()

port = int(argv[1])

nframe = 0
first = 1
perframe = 50  # only used for stats, should make it a param

anomaly = 0
correct = 0
pfalse = 0
nfalse = 0

# thresh = N,M
# flag as anomaly if key appears N times with value=1 <= M times
# thresh = 12,2 flags many false positives

nthresh = 24
mthresh = 4

kv = {}
pattern = re.compile("frame (\d+)")

sock = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
sock.bind((IP,port))

try:
  while 1:
    str = sock.recv(1024*1024)    # bufsize needs to be a param
    nframe += 1
    if first:
      match = re.search(pattern,str)
      minframe = int(match.group(1))
      first = 0
      
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

except:
  match = re.search(pattern,str)
  maxframe = int(match.group(1))
  
  print
  print "%d frames sent" % (maxframe-minframe+1)
  print "%d frames received" % nframe
  print "%g percentage received" % (100.0*nframe / (maxframe-minframe+1))
  print "%d datums received" % (nframe*perframe)
  print "%d unique keys" % len(kv)
  print "%d max occurrence of any key" % max([v[0] for v in kv.values()])
  print "%d anomalies detected" % anomaly
  print "%d true anomalies" % correct
  print "%d false positives" % pfalse
  print "%d false negatives" % nfalse
