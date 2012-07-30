import sys
import phish

# print vertices of tri in order such that vi < vj < vk

def count(nvalues):
  global n,ndup
  ndup += 1
  type,vi,tmp = phish.unpack()
  type,vj,tmp = phish.unpack()
  type,vk,tmp = phish.unpack()
  tri = [vi,vj,vk]
  tri.sort()
  if (tri[0],tri[1],tri[2]) not in hash:
    print >>fp,tri[0],tri[1],tri[2]
    hash[(tri[0],tri[1],tri[2])] = 0
    n += 1
    
def done():
  print "Triangle count %d out of %d" % (n,ndup)
  
args = phish.init(sys.argv)
phish.input(0,count,done,1)
phish.check()

fp = sys.stdout
for index,argument in enumerate(args):
  if argument == "-f":
    fp = open(args[index+1],"w")

n = ndup = 0
hash = {}

phish.loop()
phish.exit()
