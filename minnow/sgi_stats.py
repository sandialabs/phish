import sys
import phish

def count(nvalues):
  global n
  type,walk,nw = phish.unpack()
  for vi in walk: print >>fp,vi,
  print >>fp
  n += 1
    
def done():
  print "SGI count",n
  
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
