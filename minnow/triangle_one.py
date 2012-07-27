import sys
import phish

# process an edge = (Vi,Vj)
# ignore self edges
# store edge with vertex of smaller degree

def edge(nvalues):
  type,vi,tmp = phish.unpack()
  type,vj,tmp = phish.unpack()
  if vi == vj: return
  print min(vi,vj),max(vi,vj)
  if vi not in hash: di = 0
  else: di = len(hash[vi])
  if vj not in hash: dj = 0
  else: dj = len(hash[vj])
  if di == 0: hash[vi] = [vj]
  elif dj == 0: hash[vj] = [vi]
  elif di <= dj:
    if vj not in hash[vi]: hash[vi].append(vj)
  else:
    if vi not in hash[vj]: hash[vj].append(vi)
  
# process edge list to find triangles
# double loop over edges of each vertex = vj,vk
# emit triangle if find wedge-closing vj,vk edge

def find():
  for vi,value in hash.items():
    for j in xrange(len(value)):
      vj = value[j]
      if vj in hash: vjlist = hash[vj]
      else: vjlist = []
      for k in xrange(j):
        vk = value[k]
        if vk in hash: vklist = hash[vk]
        else: vklist = []
        if vk in vjlist or vj in vklist:
          phish.pack_uint64(vi)
          phish.pack_uint64(vj)
          phish.pack_uint64(vk)
          phish.send(0)

# main program
  
args = phish.init(sys.argv)
phish.input(0,edge,find,1)
phish.output(0)
phish.check()

if len(args) != 1: phish.error("Triangle_one syntax: triangle_one")
if phish.query("nlocal",0,0) != 1:
  phish.error("Can only be a single triangle_one minnow")

hash = {}

phish.loop()
phish.exit()
