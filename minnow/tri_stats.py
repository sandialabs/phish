import sys
import phish

# print vertices of tri in order such that vi < vj < vk

def count(nvalues):
  global n
  n += 1
  type,vi,tmp = phish.unpack()
  type,vj,tmp = phish.unpack()
  type,vk,tmp = phish.unpack()
  tri = [vi,vj,vk]
  tri.sort()
  print >>fp,tri[0],tri[1],tri[2]

def done():
  print "Triangle count",n
  
args = phish.init(sys.argv)
phish.input(0,count,done,1)
phish.check()

fp = sys.stdout
for index,argument in enumerate(args):
  if argument == "-f":
    fp = open(args[index+1],"w")

n = 0
phish.loop()
phish.exit()
print "TRI STATS DONE"


