import sys
import phish

# data structures:
# paths = list of [(Li,Fi),Lij,(Lj,Fj), ... ,(Ln,Fn)]
#         each entry is path thru sub-graph starting at different edge
# graph = {Vi: (Li, [Vj,...], [Lj,...], [Lij,...])} =
#         graph edges, doubly stored, with labels on verts and edges

def readpaths(file):
  txt = open(file,"r").readlines()
  paths = []
  for line in txt:
    words = line.split()
    path = []
    vflag = 1
    for word in words:
      if vflag == 0:
        if word[0] == '(' or word[-1] == ')':
          print "Syntax error in SGI path file"
          sys.exit()
        elabel = int(word)
        vflag = 1
        path.append(elabel) 
      elif vflag == 1:
        if word[0] != '(':
          print "Syntax error in SGI path file"
          sys.exit()
        vlabel = int(word[1:])
        vflag = 2
      elif vflag == 2:
        if word[-1] != ')':
          print "Syntax error in SGI path file"
          sys.exit()
        vflag = int(word[:-1])
        vflag = 0
        path.append((vlabel,vflag))
    paths.append(path)
    return paths
  
# process 1st copy of edge = (Vi,Vj,Li,Lj,Lij)
# sent by edge source to owner of Vi
# ignore self edges and duplicate edges
# store edge
# send (Vj,Vi,Lj,Li,Lij) to owner of Vj on port 1

def edge(nvalues):
  type,Vi,tmp = phish.unpack()
  type,Vj,tmp = phish.unpack()
  type,Li,tmp = phish.unpack()
  type,Lj,tmp = phish.unpack()
  type,Lij,tmp = phish.unpack()
  if Vi == Vj: return
  if Vi in graph and Vj in graph[Vi][1]: return
  if Vi not in graph: graph[Vi] = [Li,[Vj],[Lj],[Lij]]
  else:
    graph[Vi][1].append(Vj)
    graph[Vi][2].append(Lj)
    graph[Vi][3].append(Lij)
  phish.pack_uint64(Vj)
  phish.pack_uint64(Vi)
  phish.pack_uint64(Lj)
  phish.pack_uint64(Li)
  phish.pack_uint64(Lij)
  phish.send_key(1,Vj)

def edge_close():
  phish.close(1)

# process 2nd copy of edge = (Vi,Vj,Li,Lj,Lij)
# sent by another triangle minnow in edge() to owner of Vi
# store edge unless duplicate
# check if either vertex in edge matches 1st vertex of any path in paths
# for symmetric edge, only match a particular path once
# for each match, initiate a walk by sending (walkID,[V])
#   to owner of V on port 2, sending to self if V = Vi

def edge_again(nvalues):
  type,Vi,tmp = phish.unpack()
  type,Vj,tmp = phish.unpack()
  type,Li,tmp = phish.unpack()
  type,Lj,tmp = phish.unpack()
  type,Lij,tmp = phish.unpack()
  if Vi not in graph: graph[Vi] = [Li,[Vj],[Lj],[Lij]]
  elif Vj not in graph[Vi][1]:
    graph[Vi][1].append(Vj)
    graph[Vi][2].append(Lj)
    graph[Vi][3].append(Lij)
  for which,path in enumerate(paths):
    if Li == path[0][0] and Lij == path[1] and Lj == path[2][0]:
      phish.pack_int32(which)
      phish.pack_uint64_array([Vi,Vj])
      phish.send_key(2,Vj)
      continue
    if Li == path[2][0] and Lij == path[1] and Lj == path[0][0]:
      phish.pack_int32(which)
      phish.pack_uint64_array([Vj,Vi])
      phish.send_key(2,Vi)
    
# stage 0 of all walks is done, send message to port 3
      
def edge_again_close():
  phish.pack_int32(0)
  phish.send(3)

# extend walk by any neighbor vertex of last vertex in walk
# only if the neighbor vertex matches path[ipath]
# extend walk via another message, until walk is complete
# all walk datums sent and received on port 2
# SGI match message sent downstream on port 0
# important NOTE:
#   walk[-1] = vertex owned by this proc
#   but is possible proc has not yet processed an RMAT edge to store it
#   so check if walk[1] is in grpah and return if not
#   walk should still be found when edge finally arrives
  
def extend(nvalues):
  type,which,tmp = phish.unpack()
  type,walk,tmp = phish.unpack()
  n = len(walk)
  path = paths[which]
  pLij = path[2*n-1]
  pLj = path[2*n][0]
  pFj = path[2*n][1]
  if walk[-1] not in graph:
    #print "NOT FOUND",walk[1], walk
    return
  list = graph[walk[-1]]
  nedge = len(list[1])
  for i in xrange(nedge):
    Vj = list[1][i]
    Lj = list[2][i]
    Lij = list[3][i]
    if Lj != pLj or Lij != pLij: continue
    if pFj < 0 and Vj in walk: continue
    if pFj >= 0 and Vj != walk[pFj]: continue
    walk.append(Vj)
    if len(walk) <= len(path)/2:
      phish.pack_int32(which)
      phish.pack_uint64_array(walk)
      phish.send_key(1,Vj)
    else:
      phish.pack_uint64_array(walk)
      phish.send(0)
    walk.pop()

# receive done messages on port 3
# indicates sender is done with a stage of all walks
# stage is completed when recv message from all SGI procs
# key point: when a stage is complete, it means this proc
#   has received all datums and sent all extended walks for next stage,
#   thus it can send a done message for next stage
# number of stages = max number of edges in any path
# when last stage is complete, close output ports 1,2,3
# this will trigger closing of output port 0 for shutdown
    
def complete(nvalues):
  type,istage,tmp = phish.unpack()
  doneflag[istage] += 1
  if doneflag[istage] == nsgiprocs:
    if istage+1 == maxpathlen:
      phish.close(1)
      phish.close(2)
      phish.close(3)
    else:
      phish.pack_int32(istage+1)
      phish.send(3)

# main program
  
args = phish.init(sys.argv)
phish.input(0,edge,edge_close,1)
phish.input(1,edge_again,edge_again_close,1)
phish.input(2,extend,None,1)
phish.input(3,complete,None,1)
phish.output(0)
phish.output(1)
phish.output(2)
phish.output(3)
phish.check()

# read sub-graph definition

if len(args) != 2: phish.error("SGI syntax: sgi pathfile")
paths = readpaths(args[1])

# values used by complete() to track completion of path stages
# path length = edge count = len()/2 since verts are also in path
# maxpathlen = maximum length of any path

maxpathlen = 0
for path in paths: maxpathlen = max(maxpathlen,len(path)/2)
doneflag = maxpathlen*[0]
nsgiprocs = phish.query("nlocal",0,0)

graph = {}

time_start = phish.timer()

phish.loop()

time_stop = phish.timer()
str = "Elapsed time for sgi on %d procs = %g secs"
print str % (nsgiprocs,time_stop-time_start)

phish.exit()
