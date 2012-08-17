# MINNOW chain
# send messages from head to tail along a chain
# tail signals head when done

import sys,time
import phish

def loop(nvalues):
  if not tail:
    phish.repack()
    phish.send(0)
    return
  global count
  count += 1
  if count < n: return
  phish.close(0)

args = phish.init(sys.argv)
phish.input(0,loop,None,1)
phish.output(0)
phish.check()

idglobal = phish_query("idglobal",0,0)
print "PHISH host chain %d: %s" % (idglobal,phish.host())

if len(args) != 3: phish.error("Chain syntax: chain N M")
n = int(args[1])
m = int(args[2])

if n == 0: phish.error("N > 0 is required")

buf = m*"a"
count = 0

idlocal = phish.query("idlocal",0,0)
nlocal = phish.query("nlocal",0,0)
head = tail = 0
if idlocal == 0: head = 1
if idlocal == nlocal-1: tail = 1

if head:
  time_start = phish.timer()
  for i in xrange(n):
    phish.pack_string(buf)
    phish.send(0)
  phish.loop()
  time_stop = phish.timer()
  print "Elapsed time for %d chain messages of %d bytes on " + \
      "%d procs = %g secs" % (n,m,nlocal,time_stop-time_start)

else: phish.loop()
  
phish.exit()
