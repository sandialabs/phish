# MINNOW sink
# read hashed messages from source

import sys
import phish

args = phish.init(sys.argv)
phish.input(0,None,None,1)
phish.output(0)
phish.check()

idglobal = phish.query("idglobal",0,0)
print "PHISH host sink %d: %s" % (idglobal,phish.host())

if len(args) != 1: phish.error("Sink syntax: sink")

phish.loop()
phish.exit()
