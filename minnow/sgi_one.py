import sys
import phish

def error():
  phish.error("Sgi_one syntax: sgi_one -v ID label ... -e V1 V2 label ...")

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

# extend walk by one vertex if matches next stage of path
# call recursively until walk is complete

def extend(walk):
  global nextend
  nextend += 1
  p = path[len(walk)-1]
  pLij = p[2]
  pLj = p[3]
  pFj = p[4]
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
    if len(walk) <= len(path): extend(walk)
    else:
      phish.pack_uint64_array(walk)
      phish.send(0)
    walk.pop()

# process list of graph edges to find SGI matches
# if any vertex matches 1st vertex in path, initiate a walk with [Vi]
    
def find():
  p = path[0]
  for Vi,value in hash.items():
    if value[0] == p[0]: extend([Vi])
  
# main program

args = phish.init(sys.argv)
phish.input(0,edge,find,1)
phish.output(0)
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

# convert sub-graph to path, starting from first edge

hash = {}
nextend = 0

time_start = phish.timer()

phish.loop()

time_stop = phish.timer()
print "Elapsed time for sgi_one = %g secs with %d extend calls" % \
    (time_stop-time_start,nextend)

phish.exit()
