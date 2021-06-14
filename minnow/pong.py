# MINNOW pong
# reflect messages to a sender

import sys
import phish

def pong(nvalues):
  phish.repack()
  phish.send(0)

args = phish.init(sys.argv)
phish.input(0,pong,None,1)
phish.output(0)
phish.check()

idglobal = phish.query("idglobal",0,0)
print "PHISH host pong %d: %s" % (idglobal,phish.host())

if len(args) != 1: phish.error("Pong syntax: pong")

phish.loop()
phish.exit()
