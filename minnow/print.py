import sys
import phish

def myprint(nvalues):
  for i in range(nvalues):
    type,value,len = phish.unpack()
    if type == phish.RAW:
      pass
    elif type < phish.INT8_ARRAY:
      print >>fp,value,
    else:
      for val in value: print >>fp,val,
  print >>fp

argv = phish.init(sys.argv)
phish.input(0, myprint, None, 1)
phish.check()

fp = sys.stdout

for index, argument in enumerate(argv):
  if argument == "-f":
    fp = open(argv[index+1], "w")

phish.loop()
phish.exit()
