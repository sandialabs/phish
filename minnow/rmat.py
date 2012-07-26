# MINNOW rmat
# emit edges from an RMAT matrix

import sys,random
import phish

args = phish.init(sys.argv)
phish.output(0)
phish.check()

if len(args) != 9:
  phish.error("Rmat syntax: rmat N M a b c d fraction seed")

ngenerate = int(args[1]) 
nlevels = int(args[2]) 
a = float(args[3]) 
b = float(args[4]) 
c = float(args[5]) 
d = float(args[6]) 
fraction = float(args[7]) 
seed = int(args[8])

if a + b + c + d != 1.0: phish.error("Rmat a,b,c,d must sum to 1")
if fraction >= 1.0: phish.error("Rmat fraction must be < 1")
if seed <= 0: phish.error("Rmat seed must be positive integer")

# perturb seed for each minnow in case running multiple minnows

idglobal = phish.query("idglobal",0,0)
random.seed(seed+idglobal)
order = 1 << nlevels

for m in xrange(ngenerate):
  delta = order >> 1
  a1 = a; b1 = b; c1 = c; d1 = d
  i = j = 0

  for ilevel in xrange(nlevels):
    rn = random.random()
    if rn < a1: pass
    elif rn < a1+b1: j += delta
    elif rn < a1+b1+c1:	i += delta
    else:
      i += delta
      j += delta
      
    delta /= 2
    if fraction > 0.0:
      a1 += a1*fraction * (random.random() - 0.5)
      b1 += b1*fraction * (random.random() - 0.5)
      c1 += c1*fraction * (random.random() - 0.5)
      d1 += d1*fraction * (random.random() - 0.5)
      total = a1+b1+c1+d1
      a1 /= total
      b1 /= total
      c1 /= total
      d1 /= total

  phish.pack_uint64(i)
  phish.pack_uint64(j)
  #phish.send(0)
  phish.send_key(0,i)

phish.exit()
