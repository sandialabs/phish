import sys
import phish

def count(nvalues):
  global n,ndup
  ndup += 1
  type,walk,nw = phish.unpack()

  # NOTE: debug by trimming last vertex and sorting
  walk.pop(-1)
  walk.sort()
  
  if tuple(walk) not in hash:
    for vi in walk: print >>fp,vi,
    print >>fp
    hash[tuple(walk)] = 0
    n += 1
    
def done():
  print "SGI count %d out of %d" % (n,ndup)

# main program

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
