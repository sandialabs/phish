import sys
import phish

# process an edge = (Vi,Vj)
# sent by edge source to owner of Vi
# ignore self edges
# if I don't own Vi, store edge
# else send as (Vj,Vi,Di) with Di = degree of Vi, to owner of Vj on port 1

def edge(nvalues):
  type,vi,tmp = phish.unpack()
  type,vj,tmp = phish.unpack()
  if vi == vj: return
  if vi not in hash:
    hash[vi] = [vj]
  else:
    di = len(hash[vi])
    phish.pack_uint64(vj)
    phish.pack_uint64(vi)
    phish.pack_int32(di)
    phish.send_key(1,vj)

def edge_close():
  phish.close(1)
  
# process an edge + degree = (Vi,Vj,Dj)
# sent by another triangle minnow in edge() to owner of Vi
# if I don't own Vi, store edge
# else if my Di > Dj, send (Vj,Vi) back to owner of Vj on port 2
# else store edge and emit wedges
#   wedge count = N*(N-1) for N edges of Vi
#   wedge = (Vi,Vj,Vk) where Vj,Vk are edges of Vi
#   send each wedge to owners of 2 other vertices on port 3

def edge_degree(nvalues):
  type,vi,tmp = phish.unpack()
  type,vj,tmp = phish.unpack()
  type,dj,tmp = phish.unpack()
  if vi not in hash:
    hash[vi] = [vj]
  elif len(hash[vi]) > dj:
    phish.pack_uint64(vj)
    phish.pack_uint64(vi)
    phish.send_key(2,vj)
  else:
    hash[vi].append(vj)
    list = hash[vi]
    for j in xrange(len(list)):
      for k in xrange(j):
        phish.pack_uint64(vi)
        phish.pack_uint64(list[j])
        phish.pack_uint64(list[k])
        phish.send_key(3,list[j])
        phish.pack_uint64(vi)
        phish.pack_uint64(list[k])
        phish.pack_uint64(list[j])
        phish.send_key(3,list[k])

def edge_degree_close():
  phish.close(2)

# process an edge = (Vi,Vj)
# sent by another triangle minnow in edge_degree() to owner of Vi
# store edge and emit wedges
#   wedge count = N*(N-1) for N edges of Vi
#   wedge = (Vi,Vj,Vk) where Vj,Vk are edges of Vi
#   send each wedge to owners of 2 other vertices on port 3

def edge_wedge(nvalues):
  type,vi,tmp = phish.unpack()
  type,vj,tmp = phish.unpack()
  hash[vi].append(vj)
  list = hash[vi]
  for j in xrange(len(list)):
    for k in xrange(j):
      phish.pack_uint64(vi)
      phish.pack_uint64(list[j])
      phish.pack_uint64(list[k])
      phish.send_key(3,list[j])
      phish.pack_uint64(vi)
      phish.pack_uint64(list[k])
      phish.pack_uint64(list[j])
      phish.send_key(3,list[k])

def edge_wedge_close():
  phish.close(3)

# process a wedge = (Vi,Vj,Vk) where Vi is apex of wedge
# sent by another triangle minnow in edge_degree() or edge_wedge()
#   to owner of Vj (and to owner of Vk)
# if I own edge (Vj,Vk), emit triangle to port 0

def wedge(nvalues):
  type,vi,tmp = phish.unpack()
  type,vj,tmp = phish.unpack()
  type,vk,tmp = phish.unpack()
  if vj not in hash: return
  if vk not in hash[vj]: return
  phish.repack()
  phish.send(0)

# main program
  
args = phish.init(sys.argv)
phish.input(0,edge,edge_close,1)
phish.input(1,edge_degree,edge_degree_close,1)
phish.input(2,edge_wedge,edge_wedge_close,1)
phish.input(3,wedge,None,1)
phish.output(0)
phish.output(1)
phish.output(2)
phish.output(3)
phish.check()

if len(args) != 1: phish.error("Triangle syntax: triangle")

hash = {}

me = phish.query("idlocal",0,0)

phish.loop()
phish.exit()
