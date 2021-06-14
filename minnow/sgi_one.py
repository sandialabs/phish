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
        vconstraint = int(word[:-1]) - 1
        vflag = 0
        path.append((vlabel,vconstraint))
    paths.append(path)
    return paths

# process an edge = (Vi,Vj,Li,Lj,Lij)
# ignore self edges and duplicate edges
# store edge with Vi and Vj

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
  if Vj not in graph: graph[Vj] = [Lj,[Vi],[Li],[Lij]]
  else:
    graph[Vj][1].append(Vi)
    graph[Vj][2].append(Li)
    graph[Vj][3].append(Lij)

# extend walk by one vertex if matches next stage of path
# call recursively until walk is complete

def extend(which,walk):
  global nextend
  nextend += 1
  n = len(walk)
  path = paths[which]
  pLij = path[2*n-1]
  pLj = path[2*n][0]
  pFj = path[2*n][1]
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
    if len(walk) <= len(path)/2: extend(which,walk)
    else:
      phish.pack_uint64_array(walk)
      phish.send(0)
    walk.pop()

# process list of graph edges to find SGI matches
# loop over vertices and all its edges
# check if edge matches first edge of any path in paths
# only check in one direction, since Vj will check in other direction
# if edge is symmetric, avoid matching twice by only matching if Vi < Vj
# for each match, initiate a walk by calling extend with (pathID,[Vi Vj])
    
def find():
  for Vi,value in graph.items():
    Li = value[0]
    for j,Vj in enumerate(value[1]):
      Lj = value[2][j]
      Lij = value[3][j]
      for which,path in enumerate(paths):
        if Li == path[0][0] and Lij == path[1] and Lj == path[2][0]:
          if Li == Lj and Vi > Vj: continue
          extend(which,[Vi,Vj])

# main program

args = phish.init(sys.argv)
phish.input(0,edge,find,1)
phish.output(0)
phish.check()

# read sub-graph definition

if len(args) != 2: phish.error("Sgi_one syntax: sgi_one pathfile")
paths = readpaths(args[1])

# receive graph, then find SGI matches

graph = {}
nextend = 0

time_start = phish.timer()

phish.loop()

time_stop = phish.timer()
print "Elapsed time for sgi_one = %g secs with %d extend calls" % \
    (time_stop-time_start,nextend)

phish.exit()
