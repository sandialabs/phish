import sys
import phish

def error():
  phish.error("SGI syntax: sgi -v ID label ... -e V1 V2 label ...")

# data structures:
# verts = dict of (Vi : Li) for sub-graph
# edges = list of (Vi,Vj,Li,Lj,Lij) for sub-graph
# path = list of (Li,Fi,Lij,Lj,Fj) = walk thru sub-graph
# hash = {Vi: Li, [Vj,...], [Lj,...], [Lij,...]} =
#        edges in full graph, doubly stored

# process an edge = (Vi,Vj,Li,Lj,Lij)
# ignore self edges and duplicate edges
# store edge with Vi
# same edge will also be sent to Vj
# if Vi < Vj and edge matches 1st step of any walk:
#   initiate a walk by sending ([Vi]) to self on port 1

def edge(nvalues):
  type,Vi,tmp = phish.unpack()
  type,Vj,tmp = phish.unpack()
  type,Li,tmp = phish.unpack()
  type,Lj,tmp = phish.unpack()
  type,Lij,tmp = phish.unpack()
  if Vi == Vj: return
  if Vi in hash and Vj in hash[Vi][1]: return
  if Vi not in hash: hash[Vi] = [Li,[Vj],[Lj],[Lij]]
  else:
    hash[Vi][1].append(Vj)
    hash[Vi][2].append(Lj)
    hash[Vi][3].append(Lij)
  if Vi > Vj: return
  for whichpath,path in enumerate(paths):
    if Li == path[0][0]:
      phish.pack_int32(whichpath)
      phish.pack_uint64_array([Vi])
      phish.send_key(1,Vi)

# walk stage 0 is done, send message to port 2
      
def edge_close():
  phish.pack_int32(0)
  phish.send(2)

# extend walk by one vertex if matches path[ipath]
# call extend succsessively until walk is complete
# all walk datums sent and received on port 1
# SGI match message sent downstream on port 0
# important NOTE:
#   walk[-1] = vertex owned by this proc,
#   but is possible proc has not yet processed an RMAT edge to store it,
#   so check if walk[1] is in hash and return if not
  
def extend(nvalues):
  type,whichpath,tmp = phish.unpack()
  type,walk,tmp = phish.unpack()
  p = paths[whichpath][len(walk)-1]
  pLij = p[2]
  pLj = p[3]
  pFj = p[4]
  if walk[-1] not in hash:
    #print "NOT FOUND",walk[1], walk
    return
  list = hash[walk[-1]]
  nedge = len(list[1])
  for i in xrange(nedge):
    Vj = list[1][i]
    Lj = list[2][i]
    Lij = list[3][i]
    if Lj != pLj or Lij != pLij: continue
    if pFj < 0 and Vj in walk: continue
    if pFj >= 0 and Vj != walk[pFj]: continue
    walk.append(Vj)
    if len(walk) <= len(paths[whichpath]):
      phish.pack_int32(whichpath)
      phish.pack_uint64_array(walk)
      phish.send_key(1,Vj)
    else:
      phish.pack_uint64_array(walk)
      phish.send(0)
    walk.pop()

# receive done messages on port 2, indicating sender is done with a walk stage
# stage is completed when recv message from all SGI procs
# key point: when a stage is complete, it means this proc
#   has processed and sent all datums for next stage,
#   thus it can send a done message for next stage
# when last stage is complete, close input ports 1 and 2
# this will trigger closing of output port 3 for shutdown
    
def complete(nvalues):
  type,istage,tmp = phish.unpack()
  doneflag[istage] += 1
  if doneflag[istage] == nsgiprocs:
    if istage+1 == maxpathlen:
      phish.close(1)
      phish.close(2)
    else:
      phish.pack_int32(istage+1)
      phish.send(2)

# main program
  
args = phish.init(sys.argv)
phish.input(0,edge,edge_close,1)
phish.input(1,extend,None,1)
phish.input(2,complete,None,1)
phish.output(0)
phish.output(1)
phish.output(2)
phish.check()

# read sub-graph definition

if len(args) < 2: error()

verts = {}
edges = []
path = []

iarg = 1
while iarg < len(args):
  if args[iarg] == "-v":
    if iarg+3 > len(args): error()
    Vi = int(args[iarg+1])
    Li = int(args[iarg+2])
    if Vi in verts: error()
    if Li < 0: error()
    verts[Vi] = Li
    iarg += 3
  elif args[iarg] == "-e":
    if iarg+4 > len(args): error()
    Vi = int(args[iarg+1])
    Vj = int(args[iarg+2])
    Lij = int(args[iarg+3])
    if Vi not in verts or Vj not in verts: error()
    if Lij < 0: error()
    edges.append((Vi,Vj,verts[Vi],verts[Vj],Lij))
    iarg += 4
  elif args[iarg] == "-p":
    if iarg+6 > len(args): error()
    Li = int(args[iarg+1])
    Fi = int(args[iarg+2])
    Lij = int(args[iarg+3])
    Lj = int(args[iarg+4])
    Fj = int(args[iarg+5])
    path.append((Li,Fi,Lij,Lj,Fj))
    iarg += 6
  else: error()

# convert sub-graph to set of paths, starting from first edge

#paths = (path, path, path)
paths = (path,)

hash = {}

# values used by complete() to track completion of walk stages

maxpathlen = 3
doneflag = maxpathlen*[0]
nsgiprocs = phish.query("nlocal",0,0)

phish.loop()
phish.exit()
