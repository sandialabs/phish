import sys,random
import phish

def process(nvalues):
  phish.repack()
  rn = random.random()
  phish.send_key(0,rn)

args = phish.init(sys.argv)
phish.input(0,process,None,1)
phish.output(0)
phish.check()

if len(args) != 1: phish.error("Map syntax: map")

random.seed()

phish.loop()
phish.exit()
