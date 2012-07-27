import sys
import phish

# process an edge = (Vi,Vj)
# ignore self edges and duplicate edges
# store edge with both vertices

def edge(nvalues):
  type,vi,tmp = phish.unpack()
  type,vj,tmp = phish.unpack()
  if vi == vj: return
  if vi not in hash: hash[vi] = [vj]
  elif vj not in hash[vi]: hash[vi].append(vj)
  if vj not in hash: hash[vj] = [vi]
  elif vi not in hash[vj]: hash[vj].append(vi)
  
# process double edge list to find triangles
# double loop over edges of each vertex = vj,vk
# look for wedge-closing vj,vk edges
# only emit triangle if vi is smallest vertex to avoid duplicates

def find():
  for vi,list in hash.items():
    for j in xrange(len(list)):
      vj = list[j]
      if vj in hash: vjlist = hash[vj]
      else: vjlist = []
      for k in xrange(j):
        vk = list[k]
        if vk in vjlist:
          if vi > vj or vi > vk: continue
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
