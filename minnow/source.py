import sys,random
import phish

args = phish.init(sys.argv)
phish.input(0,None,None,1)
phish.output(0)
phish.check()

if len(args) != 2: phish.error("Source syntax: source N M")

n = int(args[0])
m = int(args[1])

idlocal = phish.query("idlocal",0,0)
nlocal = phish.query("nlocal",0,0)
nme = n / nlocal;
if idlocal < n % nlocal: nme += 1

buf = m*"a"
random.seed()

time_start = phish.timer()

for i in xrange(nme):
  phish.pack_string(buf)
  rn = random.random()
  phish.send_key(0,rn)

phish.close(0)
phish.loop()

time_stop = phish.timer()
print "Elapsed time for %d hashed messages of %d bytes = %g secs" % \
    (n,m,time_stop-time_start)

phish.exit()
