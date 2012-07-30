import sys
import phish

# process an edge = (Vi,Vj)
# sent by edge source to owner of Vi
# ignore self edges and duplicate edges
# store edge and compute Di = degree of Vi
# send (Vj,Vi,Di) to owner of Vj on port 1

def edge(nvalues):
  type,vi,tmp = phish.unpack()
  type,vj,tmp = phish.unpack()
  if vi == vj: return
  if vi in hash and vj in hash[vi]: return
  if vi not in hash: hash[vi] = [vj]
  else: hash[vi].append(vj)
  di = len(hash[vi])
  phish.pack_uint64(vj)
  phish.pack_uint64(vi)
  phish.pack_int32(di)
  phish.send_key(1,vj)

def edge_close():
  phish.close(1)
  
# process an edge + degree = (Vi,Vj,Dj)
# sent by another triangle minnow in edge() to owner of Vi
# store edge unless duplicate
# compute Di = degree of Vi
# if Di >= Dj, send (Vj,Vi) to owner of Vj on port 2 as neighbor request
# else Di < Dj, so send (Vj,Vi,Ni) to owner of Vj on port 3
#   Ni = neighbor list of Vi

def edge_degree(nvalues):
  type,vi,tmp = phish.unpack()
  type,vj,tmp = phish.unpack()
  type,dj,tmp = phish.unpack()
  if vi not in hash: hash[vi] = [vj]
  elif vj not in hash[vi]: hash[vi].append(vj)
  di = len(hash[vi])
  if di >= dj:
    phish.pack_uint64(vj)
    phish.pack_uint64(vi)
    phish.send_key(2,vj)
  else:
    phish.pack_uint64(vj)
    phish.pack_uint64(vi)
    phish.pack_uint64_array(hash[vi])
    phish.send_key(3,vj)

def edge_degree_close():
  phish.close(2)

# process a neighbor request = (Vi,Vj)
# sent by another triangle minnow in edge_degree() to owner of Vi
# send (Vj,Vi,Ni) to owner of Vj on port 3

def neigh_request(nvalues):
  type,vi,tmp = phish.unpack()
  type,vj,tmp = phish.unpack()
  phish.pack_uint64(vj)
  phish.pack_uint64(vi)
  phish.pack_uint64_array(hash[vi])
  phish.send_key(3,vj)

def neigh_request_close():
  phish.close(3)

# process a neighbor list = (Vi,Vj,Nj) where Nj = neighbor list of Vj
# Nj is smaller than Ni, so loop over Nj
# if value in Nj is in Ni, emit found triangle to port 0

def neighbors(nvalues):
  type,vi,tmp = phish.unpack()
  type,vj,tmp = phish.unpack()
  type,nj,tmp = phish.unpack()
  ni = hash[vi]
  #print "NEIGH",vi,vj,ni,nj
  for vk in nj:
    if vk in ni:
      phish.pack_uint64(vi)
      phish.pack_uint64(vj)
      phish.pack_uint64(vk)
      phish.send(0)

# main program
  
args = phish.init(sys.argv)
phish.input(0,edge,edge_close,1)
phish.input(1,edge_degree,edge_degree_close,1)
phish.input(2,neigh_request,neigh_request_close,1)
phish.input(3,neighbors,None,1)
phish.output(0)
phish.output(1)
phish.output(2)
phish.output(3)
phish.check()

if len(args) != 1: phish.error("Triangle syntax: triangle")

hash = {}

phish.loop()
phish.exit()
