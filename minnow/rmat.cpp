// MINNOW rmat
// emit edges from an RMAT matrix as (I,J)
// optional -o switch = std, hash, double
//   std = send once, unhashed (default)
//   hash = send once, hashed to owner of vertex I
//   double = send twice, hashed to owner of vertex I and vertex J
// optional -V and -E switches, default for both is 0
//   if V > 0, add random vertex labels from 1 to V inclusive
//     send edge as (I,J,Vi,Vj)
//   if E > 0, add random edge label from 1 to E inclusive
//     send edge as (I,J,Vij)
//   if both V and E > 0, add both kinds of labels
//     send edge as (I,J,Vi,Vj,Vij)

#include "stdint.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "phish.h"

uint32_t hashlittle(const void *key, size_t length, uint32_t);
void error() {
  phish_error("Rmat syntax: rmat N M a b c d fraction seed -o mode -v V -e E");
}

/* ---------------------------------------------------------------------- */

int main(int narg, char **args)
{
  phish_init(&narg,&args);
  phish_output(0);
  phish_check();

  int idglobal = phish_query("idglobal",0,0);
  printf("PHISH host rmat %d: %s\n",idglobal,phish_host());

  if (narg < 9) error();

  uint64_t ngenerate = atol(args[1]); 
  int nlevels = atoi(args[2]); 
  double a = atof(args[3]); 
  double b = atof(args[4]); 
  double c = atof(args[5]); 
  double d = atof(args[6]); 
  double fraction = atof(args[7]); 
  int seed = atoi(args[8]);

  // optional args

  int mode = 0;
  int vlabel = 0;
  int elabel = 0;
  
  int iarg = 9;
  while (iarg < narg) {
    if (strcmp(args[iarg],"-o") == 0) {
      if (iarg+2 > narg) error();
      if (strcmp(args[iarg+1],"std") == 0) mode = 0;
      else if (strcmp(args[iarg+1],"hash") == 0) mode = 1;
      else if (strcmp(args[iarg+1],"double") == 0) mode = 2;
      else error();
      iarg += 2;
    } else if (strcmp(args[iarg],"-v") == 0) {
      if (iarg+2 > narg) error();
      vlabel = atoi(args[iarg+1]);
      iarg += 2;
    } else if (strcmp(args[iarg],"-e") == 0) {
      if (iarg+2 > narg) error();
      elabel = atoi(args[iarg+1]);
      iarg += 2;
    } else error();
  }

  if (a + b + c + d != 1.0) phish_error("Rmat a,b,c,d must sum to 1");
  if (fraction >= 1.0) phish_error("Rmat fraction must be < 1");
  if (seed <= 0) phish_error("Rmat seed must be positive integer");
  if (vlabel < 0) phish_error("Invalid -v value");
  if (elabel < 0) phish_error("Invalid -e value");

  // perturb seed for each minnow in case running multiple minnows
  // divide ngenerate by multiple minnows

  int idlocal = phish_query("idlocal",0,0);
  int nlocal = phish_query("nlocal",0,0);

  srand48(seed+idglobal);
  uint64_t order = 1L << nlevels;

  uint64_t nme = ngenerate / nlocal;
  if (idlocal < ngenerate % nlocal) nme++;

  // generate edges = (Vi,Vj)
  // append vertex and edge labels if requested: (Vi,Vj,Li,Lj,Lij)
  // send each edge in one of 3 different modes:
  // standard = send edge once via standard, unhashed send
  // hashed = send edge once, hashed on Vi
  // double = send edge twice, hashed on Vi and on Vj

  uint64_t i,j,delta,sum;
  int ilevel,ilabel,jlabel,ijlabel;
  double a1,b1,c1,d1,total,rn;

  double time_start = phish_timer();

  for (uint64_t m = 0; m < nme; m++) {
    if (m % 1000000 == 0) printf("RMAT %d: %ld\n",idlocal,m);
    delta = order >> 1;
    a1 = a; b1 = b; c1 = c; d1 = d;
    i = j = 0;

    for (ilevel = 0; ilevel < nlevels; ilevel++) {
      rn = drand48();
      if (rn < a1) {
      } else if (rn < a1+b1) {
	j += delta;
      } else if (rn < a1+b1+c1) {
	i += delta;
      } else {
	i += delta;
	j += delta;
      }
      
      delta /= 2;
      if (fraction > 0.0) {
	a1 += a1*fraction * (drand48() - 0.5);
	b1 += b1*fraction * (drand48() - 0.5);
	c1 += c1*fraction * (drand48() - 0.5);
	d1 += d1*fraction * (drand48() - 0.5);
	total = a1+b1+c1+d1;
	a1 /= total;
	b1 /= total;
	c1 /= total;
	d1 /= total;
      }
    }

    phish_pack_uint64(i);
    phish_pack_uint64(j);

    // use hashlittle() to generate random labels
    // require label of a vertex or edge always be consistent
  
    if (vlabel) {
      int ilabel = hashlittle((char *) &i,sizeof(uint64_t),0) % vlabel + 1;
      int jlabel = hashlittle((char *) &j,sizeof(uint64_t),0) % vlabel + 1;
      phish_pack_uint64(ilabel);
      phish_pack_uint64(jlabel);
    }
    if (elabel) {
      sum = i + j;
      int ijlabel = hashlittle((char *) &sum,sizeof(uint64_t),0) % elabel + 1;
      phish_pack_uint64(ijlabel);
    }

    if (mode == 0) phish_send(0);
    else {
      phish_send_key(0,(char *) &i,sizeof(uint64_t));
      if (mode == 2) {
        phish_pack_uint64(j);
        phish_pack_uint64(i);
        if (vlabel) {
          phish_pack_uint64(jlabel);
          phish_pack_uint64(ilabel);
        }
        if (elabel) phish_pack_uint64(ijlabel);
        phish_send_key(0,(char *) &j,sizeof(uint64_t));
      }
    }
  }
  
  double time_stop = phish_timer();
  if (idlocal == 0)
    printf("Elapsed time for rmat of %ld edges on %d procs = %g secs\n",
           ngenerate,nlocal,time_stop-time_start);

  phish_exit();
}

/* ---------------------------------------------------------------------- */

// Hash function hashlittle()
// from lookup3.c, by Bob Jenkins, May 2006, Public Domain
// bob_jenkins@burtleburtle.net

#include "stddef.h"
#include "stdint.h"

#define HASH_LITTLE_ENDIAN 1       // Intel and AMD are little endian

#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

/*
-------------------------------------------------------------------------------
mix -- mix 3 32-bit values reversibly.

This is reversible, so any information in (a,b,c) before mix() is
still in (a,b,c) after mix().

If four pairs of (a,b,c) inputs are run through mix(), or through
mix() in reverse, there are at least 32 bits of the output that
are sometimes the same for one pair and different for another pair.
This was tested for:
* pairs that differed by one bit, by two bits, in any combination
  of top bits of (a,b,c), or in any combination of bottom bits of
  (a,b,c).
* "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
  the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
  is commonly produced by subtraction) look like a single 1-bit
  difference.
* the base values were pseudorandom, all zero but one bit set, or 
  all zero plus a counter that starts at zero.

Some k values for my "a-=c; a^=rot(c,k); c+=b;" arrangement that
satisfy this are
    4  6  8 16 19  4
    9 15  3 18 27 15
   14  9  3  7 17  3
Well, "9 15 3 18 27 15" didn't quite get 32 bits diffing
for "differ" defined as + with a one-bit base and a two-bit delta.  I
used http://burtleburtle.net/bob/hash/avalanche.html to choose 
the operations, constants, and arrangements of the variables.

This does not achieve avalanche.  There are input bits of (a,b,c)
that fail to affect some output bits of (a,b,c), especially of a.  The
most thoroughly mixed value is c, but it doesn't really even achieve
avalanche in c.

This allows some parallelism.  Read-after-writes are good at doubling
the number of bits affected, so the goal of mixing pulls in the opposite
direction as the goal of parallelism.  I did what I could.  Rotates
seem to cost as much as shifts on every machine I could lay my hands
on, and rotates are much kinder to the top and bottom bits, so I used
rotates.
-------------------------------------------------------------------------------
*/
#define mix(a,b,c) \
{ \
  a -= c;  a ^= rot(c, 4);  c += b; \
  b -= a;  b ^= rot(a, 6);  a += c; \
  c -= b;  c ^= rot(b, 8);  b += a; \
  a -= c;  a ^= rot(c,16);  c += b; \
  b -= a;  b ^= rot(a,19);  a += c; \
  c -= b;  c ^= rot(b, 4);  b += a; \
}

/*
-------------------------------------------------------------------------------
final -- final mixing of 3 32-bit values (a,b,c) into c

Pairs of (a,b,c) values differing in only a few bits will usually
produce values of c that look totally different.  This was tested for
* pairs that differed by one bit, by two bits, in any combination
  of top bits of (a,b,c), or in any combination of bottom bits of
  (a,b,c).
* "differ" is defined as +, -, ^, or ~^.  For + and -, I transformed
  the output delta to a Gray code (a^(a>>1)) so a string of 1's (as
  is commonly produced by subtraction) look like a single 1-bit
  difference.
* the base values were pseudorandom, all zero but one bit set, or 
  all zero plus a counter that starts at zero.

These constants passed:
 14 11 25 16 4 14 24
 12 14 25 16 4 14 24
and these came close:
  4  8 15 26 3 22 24
 10  8 15 26 3 22 24
 11  8 15 26 3 22 24
-------------------------------------------------------------------------------
*/
#define final(a,b,c) \
{ \
  c ^= b; c -= rot(b,14); \
  a ^= c; a -= rot(c,11); \
  b ^= a; b -= rot(a,25); \
  c ^= b; c -= rot(b,16); \
  a ^= c; a -= rot(c,4);  \
  b ^= a; b -= rot(a,14); \
  c ^= b; c -= rot(b,24); \
}

/*
-------------------------------------------------------------------------------
hashlittle() -- hash a variable-length key into a 32-bit value
  k       : the key (the unaligned variable-length array of bytes)
  length  : the length of the key, counting by bytes
  initval : can be any 4-byte value
Returns a 32-bit value.  Every bit of the key affects every bit of
the return value.  Two keys differing by one or two bits will have
totally different hash values.

The best hash table sizes are powers of 2.  There is no need to do
mod a prime (mod is sooo slow!).  If you need less than 32 bits,
use a bitmask.  For example, if you need only 10 bits, do
  h = (h & hashmask(10));
In which case, the hash table should have hashsize(10) elements.

If you are hashing n strings (uint8_t **)k, do it like this:
  for (i=0, h=0; i<n; ++i) h = hashlittle( k[i], len[i], h);

By Bob Jenkins, 2006.  bob_jenkins@burtleburtle.net.  You may use this
code any way you wish, private, educational, or commercial.  It's free.

Use for hash table lookup, or anything where one collision in 2^^32 is
acceptable.  Do NOT use for cryptographic purposes.
-------------------------------------------------------------------------------
*/

uint32_t hashlittle( const void *key, size_t length, uint32_t initval)
{
#ifndef PURIFY_HATES_HASHLITTLE

  uint32_t a,b,c;                                          /* internal state */
  union { const void *ptr; size_t i; } u;     /* needed for Mac Powerbook G4 */

  /* Set up the internal state */
  a = b = c = 0xdeadbeef + ((uint32_t)length) + initval;

  u.ptr = key;
  if (HASH_LITTLE_ENDIAN && ((u.i & 0x3) == 0)) {
    const uint32_t *k = (const uint32_t *)key;         /* read 32-bit chunks */
    const uint8_t  *k8;

    /*------ all but last block: aligned reads and affect 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      b += k[1];
      c += k[2];
      mix(a,b,c);
      length -= 12;
      k += 3;
    }

    /*----------------------------- handle the last (probably partial) block */
    /* 
     * "k[2]&0xffffff" actually reads beyond the end of the string, but
     * then masks off the part it's not allowed to read.  Because the
     * string is aligned, the masked-off tail is in the same word as the
     * rest of the string.  Every machine with memory protection I've seen
     * does it on word boundaries, so is OK with this.  But VALGRIND will
     * still catch it and complain.  The masking trick does make the hash
     * noticably faster for short strings (like English words).
     */
#ifndef VALGRIND

    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=k[2]&0xffffff; b+=k[1]; a+=k[0]; break;
    case 10: c+=k[2]&0xffff; b+=k[1]; a+=k[0]; break;
    case 9 : c+=k[2]&0xff; b+=k[1]; a+=k[0]; break;
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=k[1]&0xffffff; a+=k[0]; break;
    case 6 : b+=k[1]&0xffff; a+=k[0]; break;
    case 5 : b+=k[1]&0xff; a+=k[0]; break;
    case 4 : a+=k[0]; break;
    case 3 : a+=k[0]&0xffffff; break;
    case 2 : a+=k[0]&0xffff; break;
    case 1 : a+=k[0]&0xff; break;
    case 0 : return c;              /* zero length strings require no mixing */
    }

#else /* make valgrind happy */

    k8 = (const uint8_t *)k;
    switch(length)
    {
    case 12: c+=k[2]; b+=k[1]; a+=k[0]; break;
    case 11: c+=((uint32_t)k8[10])<<16;  /* fall through */
    case 10: c+=((uint32_t)k8[9])<<8;    /* fall through */
    case 9 : c+=k8[8];                   /* fall through */
    case 8 : b+=k[1]; a+=k[0]; break;
    case 7 : b+=((uint32_t)k8[6])<<16;   /* fall through */
    case 6 : b+=((uint32_t)k8[5])<<8;    /* fall through */
    case 5 : b+=k8[4];                   /* fall through */
    case 4 : a+=k[0]; break;
    case 3 : a+=((uint32_t)k8[2])<<16;   /* fall through */
    case 2 : a+=((uint32_t)k8[1])<<8;    /* fall through */
    case 1 : a+=k8[0]; break;
    case 0 : return c;
    }

#endif /* !valgrind */

  } else if (HASH_LITTLE_ENDIAN && ((u.i & 0x1) == 0)) {
    const uint16_t *k = (const uint16_t *)key;         /* read 16-bit chunks */
    const uint8_t  *k8;

    /*--------------- all but last block: aligned reads and different mixing */
    while (length > 12)
    {
      a += k[0] + (((uint32_t)k[1])<<16);
      b += k[2] + (((uint32_t)k[3])<<16);
      c += k[4] + (((uint32_t)k[5])<<16);
      mix(a,b,c);
      length -= 12;
      k += 6;
    }

    /*----------------------------- handle the last (probably partial) block */
    k8 = (const uint8_t *)k;
    switch(length)
    {
    case 12: c+=k[4]+(((uint32_t)k[5])<<16);
             b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 11: c+=((uint32_t)k8[10])<<16;     /* fall through */
    case 10: c+=k[4];
             b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 9 : c+=k8[8];                      /* fall through */
    case 8 : b+=k[2]+(((uint32_t)k[3])<<16);
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 7 : b+=((uint32_t)k8[6])<<16;      /* fall through */
    case 6 : b+=k[2];
             a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 5 : b+=k8[4];                      /* fall through */
    case 4 : a+=k[0]+(((uint32_t)k[1])<<16);
             break;
    case 3 : a+=((uint32_t)k8[2])<<16;      /* fall through */
    case 2 : a+=k[0];
             break;
    case 1 : a+=k8[0];
             break;
    case 0 : return c;                     /* zero length requires no mixing */
    }

  } else {                        /* need to read the key one byte at a time */
    const uint8_t *k = (const uint8_t *)key;

    /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
    while (length > 12)
    {
      a += k[0];
      a += ((uint32_t)k[1])<<8;
      a += ((uint32_t)k[2])<<16;
      a += ((uint32_t)k[3])<<24;
      b += k[4];
      b += ((uint32_t)k[5])<<8;
      b += ((uint32_t)k[6])<<16;
      b += ((uint32_t)k[7])<<24;
      c += k[8];
      c += ((uint32_t)k[9])<<8;
      c += ((uint32_t)k[10])<<16;
      c += ((uint32_t)k[11])<<24;
      mix(a,b,c);
      length -= 12;
      k += 12;
    }

    /*-------------------------------- last block: affect all 32 bits of (c) */
    switch(length)                   /* all the case statements fall through */
    {
    case 12: c+=((uint32_t)k[11])<<24;
    case 11: c+=((uint32_t)k[10])<<16;
    case 10: c+=((uint32_t)k[9])<<8;
    case 9 : c+=k[8];
    case 8 : b+=((uint32_t)k[7])<<24;
    case 7 : b+=((uint32_t)k[6])<<16;
    case 6 : b+=((uint32_t)k[5])<<8;
    case 5 : b+=k[4];
    case 4 : a+=((uint32_t)k[3])<<24;
    case 3 : a+=((uint32_t)k[2])<<16;
    case 2 : a+=((uint32_t)k[1])<<8;
    case 1 : a+=k[0];
             break;
    case 0 : return c;
    }
  }

  final(a,b,c);
  return c;

#else  /* PURIFY_HATES_HASHLITTLE */
/* I don't know what it is about Jenkins' hashlittle function, but
 * it drives purify insane, even with VALGRIND defined.  It makes 
 * purify unusable!!  The code execution doesn't even make sense.
 * Below is a (probably) weaker hash function that at least allows 
 * testing with purify.
 */
#define MAXINT_DIV_PHI  11400714819323198485U

  uint32_t h, rest, *p, bytes, num_bytes;
  char *byteptr;

  num_bytes = length;

  /* First hash the uint32_t-sized portions of the key */
  h = 0;
  for (p = (uint32_t *)key, bytes=num_bytes;
       bytes >= (uint32_t) sizeof(uint32_t);
       bytes-=sizeof(uint32_t), p++){
    h = (h^(*p))*MAXINT_DIV_PHI;
  }

  /* Then take care of the remaining bytes, if any */
  rest = 0;
  for (byteptr = (char *)p; bytes > 0; bytes--, byteptr++){
    rest = (rest<<8) | (*byteptr);
  }

  /* If extra bytes, merge the two parts */
  if (rest)
    h = (h^rest)*MAXINT_DIV_PHI;

  return h;
#endif /* PURIFY_HATES_HASHLITTLE */
}
