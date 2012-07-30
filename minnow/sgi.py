import sys
import phish

def error():
  phish.error("SGI syntax: sgi -v ID label ... -e V1 V2 label ...")

# data structures:
# verts = dict of (Vi : Li) for sub-graph
# edges = list of (Vi,Vj,Li,Lj,Lij) for sub-graph
# path = list of (Li,Fi,Lij,Lj,Fj) = walk thru sub-graph
# hash = (Vi: Li, [Vj,...], [Lj,...], [Lij,...]) =
#        edges in full graph, doubly stored

# process an edge = (Vi,Vj,Li,Lj,Lij)
# ignore self edges and duplicate edges
# store edge with Vi
# same edge will also be sent to Vj
# if Vi < Vj and edge matches 1st step of any walk, initiate a walk
# send (??) to all valid Vj on port 1

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
  if vi > vj: return
  for ipath,path in enumerate(paths):
    if Li == path[0]:
      phish.pack_int(ipath)
      phish.pack_int(0)
      phish.pack_uint64_array([Vi])
      phish.send_key(1,Vi)
  
def edge_close():
  #phish.close(1)
  pass

# extend walk by one vertex if matches path[ipath]
# call recursively until walk is complete

def extend(nvalues):
  type,wpath,tmp = phish.unpack()
  type,ipath,tmp = phish.unpack()
  type,walk,tmp = phish.unpack()
  path = paths[wpath]
  pLij = path[ipath][2]
  pLj = path[ipath][3]
  pFj = path[ipath][4]
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
    phish.pack_int(ipath)
    phish.pack_int(0)
    phish.pack_uint64_array(walk)
    phish.send_key(1,Vi)
    walk.pop()
  
# main program
  
args = phish.init(sys.argv)
phish.input(0,edge,edge_close,1)
phish.input(1,extend,None,1)
phish.output(0)
phish.output(1)
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

paths = (path, path, path)
  
hash = {}

phish.loop()
phish.exit()
