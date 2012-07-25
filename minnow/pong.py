import sys
import phish

def pong(nvalues):
  phish.repack()
  phish.send(0)

args = phish.init(sys.argv)
phish.input(0,pong,None,1)
phish.output(0)
phish.check()

if len(args) != 1: phish.error("Pong syntax: pong")

phish.loop()
phish.exit()
