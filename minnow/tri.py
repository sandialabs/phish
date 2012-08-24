import sys
import phish

# process 1st copy of edge = (Vi,Vj)
# sent by edge source to owner of Vi
# ignore self edges and duplicate edges
# store edge
# send (Vj,Vi) to owner of Vj on port 1

def edge(nvalues):
  type,vi,tmp = phish.unpack()
  type,vj,tmp = phish.unpack()
  if vi == vj: return
  if vi in hash and vj in hash[vi]: return
  if vi not in hash: hash[vi] = [vj]
  else: hash[vi].append(vj)
  phish.pack_uint64(vj)
  phish.pack_uint64(vi)
  phish.send_key(1,vj)

def edge_close():
  phish.close(1)
  
# process 2nd copy of edge = (Vi,Vj)
# sent by another triangle minnow in edge() to owner of Vi
# store edge unless duplicate
# Di = degree of Vi
# send (Vj,Vi,Di) to owner of Vj on port 2

def edge_again(nvalues):
  type,vi,tmp = phish.unpack()
  type,vj,tmp = phish.unpack()
  if vi not in hash: hash[vi] = [vj]
  elif vj not in hash[vi]: hash[vi].append(vj)
  di = len(hash[vi])
  phish.pack_uint64(vj)
  phish.pack_uint64(vi)
  phish.pack_int32(di)
  phish.send_key(2,vj)

def edge_again_close():
  phish.close(2)
  
# process an edge + degree = (Vi,Vj,Dj)
# sent by another triangle minnow in edge_again() to owner of Vi
# Di = degree of Vi
# if Di > Dj, send (Vj,Vi,Dj,Di) as neigh request
#   for Dj neighbors to owner of Vj on port 3
# else Di <= Dj, so send (Vj,Vi,Dj,Ni) to owner of Vj on port 4
#   Ni = neighbor list of Vi

def edge_degree(nvalues):
  type,vi,tmp = phish.unpack()
  type,vj,tmp = phish.unpack()
  type,dj,tmp = phish.unpack()
  di = len(hash[vi])
  if di > dj:
    phish.pack_uint64(vj)
    phish.pack_uint64(vi)
    phish.pack_uint64(dj)
    phish.pack_uint64(di)
    phish.send_key(3,vj)
  else:
    phish.pack_uint64(vj)
    phish.pack_uint64(vi)
    phish.pack_uint64(dj)
    phish.pack_uint64_array(hash[vi])
    phish.send_key(4,vj)

def edge_degree_close():
  phish.close(3)

# process a neighbor request = (Vi,Vj,Di,Dj)
# sent by another triangle minnow in edge_degree() to owner of Vi
# send (Vj,Vi,Dj,Ni) to owner of Vj on port 4
# only send first Di neighbors of Ni

def neigh_request(nvalues):
  type,vi,tmp = phish.unpack()
  type,vj,tmp = phish.unpack()
  type,di,tmp = phish.unpack()
  type,dj,tmp = phish.unpack()
  phish.pack_uint64(vj)
  phish.pack_uint64(vi)
  phish.pack_uint64(dj)
  phish.pack_uint64_array(hash[vi][:di])
  phish.send_key(4,vj)

def neigh_request_close():
  phish.close(4)

# process a neighbor list = (Vi,Vj,Di,Nj) where Nj = neighbor list of Vj
# Nj is <= than first Di neighbors of Ni, so loop over Nj, not Ni
# if value in Nj is also in Ni, emit found triangle to port 0

def neighbors(nvalues):
  type,vi,tmp = phish.unpack()
  type,vj,tmp = phish.unpack()
  type,di,tmp = phish.unpack()
  type,nj,tmp = phish.unpack()
  ni = hash[vi][:di]
  for vk in nj:
    if vk in ni:
      phish.pack_uint64(vi)
      phish.pack_uint64(vj)
      phish.pack_uint64(vk)
      phish.send(0)

# main program
  
args = phish.init(sys.argv)
phish.input(0,edge,edge_close,1)
phish.input(1,edge_again,edge_again_close,1)
phish.input(2,edge_degree,edge_degree_close,1)
phish.input(3,neigh_request,neigh_request_close,1)
phish.input(4,neighbors,None,1)
phish.output(0)
phish.output(1)
phish.output(2)
phish.output(3)
phish.output(4)
phish.check()

if len(args) != 1: phish.error("Tri syntax: tri")

hash = {}

nlocal = phish.query("nlocal",0,0)
time_start = phish.timer()

phish.loop()

time_stop = phish.timer()
str = "Elapsed time for tri on %d procs = %g secs"
print str % (nlocal,time_stop-time_start)

phish.exit()
