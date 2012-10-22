#!/usr/local/bin/python

# MINNOW edge
# receive and store a graph edge
# mode = 0, don't store at all
# mode = 1, store once as (Vi,Vj)
# mode = 2, store once as (Vi,Vj), send to Vj owner, store again as (Vj,Vi)

import sys
import phish

# process 1st copy of edge = (Vi,Vj)
# sent by edge source to owner of Vi
# ignore self edges and duplicate edges
# store edge

def edge_once(nvalues):
  global nedge
  type,vi,tmp = phish.unpack()
  type,vj,tmp = phish.unpack()
  if vi == vj: return
  if vi in graph and vj in graph[vi]: return
  if vi not in graph: graph[vi] = [vj]
  else: graph[vi].append(vj)
  nedge += 1

# process 1st copy of edge = (Vi,Vj)
# sent by edge source to owner of Vi
# ignore self edges and duplicate edges
# store edge
# send (Vj,Vi) to owner of Vj on port 1

def edge_first(nvalues):
  global nedge
  type,vi,tmp = phish.unpack()
  type,vj,tmp = phish.unpack()
  if vi == vj: return
  if vi in graph and vj in graph[vi]: return
  if vi not in graph: graph[vi] = [vj]
  else: graph[vi].append(vj)
  nedge += 1
  phish.pack_uint64(vj)
  phish.pack_uint64(vi)
  phish.send_key(1,vj)

# process 2nd copy of edge = (Vi,Vj)
# sent by another edge minnow in edge_first() to owner of Vi
# store edge unless duplicate

def edge_second(nvalues):
  global nedge
  type,vi,tmp = phish.unpack()
  type,vj,tmp = phish.unpack()
  if vi not in graph:
    graph[vi] = [vj]
    nedge += 1
  elif vj not in graph[vi]:
    graph[vi].append(vj)
    nedge += 1

# close port to self
  
def edge_close():
  phish.close(1)

# main program
  
args = phish.init(sys.argv)

idglobal = phish.query("idglobal",0,0)
print "PHISH host edge %d: %s" % (idglobal,phish.host())

if len(args) != 2: phish.error("Edge syntax: edge 0/1/2")
mode = int(args[1])
if mode < 0 or mode > 2: phish.error("Edge syntax: edge 0/1/2")

if mode == 0:
  phish.input(0,None,edge_close,0)
  phish.input(1,None,None,0)
elif mode == 1:
  phish.input(0,edge_once,edge_close,1)
  phish.input(1,None,None,0)
elif mode == 2:
  phish.input(0,edge_first,edge_close,1)
  phish.input(1,edge_second,None,1)

phish.output(0)
phish.output(1)
phish.check()

graph = {}
nedge = 0

nlocal = phish.query("nlocal",0,0)
time_start = phish.timer()

phish.loop()

time_stop = phish.timer()
str = "Elapsed time for edge storage of %d edges on %d procs = %g secs\n"
print str % (nedge,nlocal,time_stop-time_start,nedge)

phish.exit()
