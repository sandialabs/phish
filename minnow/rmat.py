# MINNOW rmat
# emit edges from an RMAT matrix

import sys,random
import phish

def error():
  phish.error("Rmat syntax: rmat N M a b c d fraction seed -o mode -v V -e E")

# Park/Miller RNG, called with explicit seed each time

IA = 16807
IM = 2147483647
AM = 1.0/IM
IQ = 127773
IR = 2836

def parkRNG(seed):
  seed = seed & IM
  k = int(seed/IQ)
  seed = IA*(seed-k*IQ) - IR*k
  if seed < 0: seed += IM
  ans = AM*seed
  return ans

# main program

args = phish.init(sys.argv)
phish.output(0)
phish.check()

# mandatory args

if len(args) < 9: error()

ngenerate = int(args[1]) 
nlevels = int(args[2]) 
a = float(args[3]) 
b = float(args[4]) 
c = float(args[5]) 
d = float(args[6]) 
fraction = float(args[7]) 
seed = int(args[8])

# optional args

mode = 0
vlabel = elabel = 0

iarg = 9
while iarg < len(args):
  if args[iarg] == "-o":
    if iarg+2 > len(args): error()
    if args[iarg+1] == "standard": mode = 0
    elif args[iarg+1] == "hashed": mode = 1
    elif args[iarg+1] == "double": mode = 2
    else: error()
    iarg += 2
  elif args[iarg] == "-v":
    if iarg+2 > len(args): error()
    vlabel = int(args[iarg+1])
    iarg += 2
  elif args[iarg] == "-e":
    if iarg+2 > len(args): error()
    elabel = int(args[iarg+1])
    iarg += 2
  else: error()

if a + b + c + d != 1.0: phish.error("Rmat a,b,c,d must sum to 1")
if fraction >= 1.0: phish.error("Rmat fraction must be < 1")
if seed <= 0: phish.error("Rmat seed must be positive integer")
if vlabel < 0: phish.error("Invalid -v value")
if elabel < 0: phish.error("Invalid -e value")

# perturb seed for each minnow in case running multiple minnows

idglobal = phish.query("idglobal",0,0)
random.seed(seed+idglobal)
order = 1 << nlevels

# generate edges = (Vi,Vj)
# append vertex and edge labels if requested: (Vi,Vj,Li,Lj,Lij)
# send each edge in one of 3 different modes:
# standard = send edge once via standard, unhashed send
# hashed = send edge once, hashed on Vi
# double = send edge twice, hashed on Vi and on Vj

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

  #if m == 0:
  #  i = 1; j = 2
  #elif m == 1:
  #  i = 1; j = 3
  #elif m == 2:
  #  i = 2; j = 3
    
  phish.pack_uint64(i)
  phish.pack_uint64(j)
  
  if vlabel:
    ilabel = int(vlabel*parkRNG(i))
    jlabel = int(vlabel*parkRNG(j))
    phish.pack_uint64(ilabel)
    phish.pack_uint64(jlabel)
  if elabel:
    ijlabel = int(elabel*parkRNG(i+j))
    phish.pack_uint64(ijlabel)

  if mode == 0: phish.send(0)
  else:
    phish.send_key(0,long(i))
    if mode == 2:
      phish.pack_uint64(j)
      phish.pack_uint64(i)
      if vlabel:
        phish.pack_uint64(jlabel)
        phish.pack_uint64(ilabel)
      if elabel: phish.pack_uint64(ijlabel)
      phish.send_key(0,long(j))
    
phish.exit()
