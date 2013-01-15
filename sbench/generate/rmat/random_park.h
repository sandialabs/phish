#ifndef _RANDOM_PARK_H
#define _RANDOM_PARK_H 

// Park/Miller random number generator (rng)
// multiplicative linear congruential generator
// SK Park and KW Miller, "Random Number Generators: Good Ones Are
//   Hard To Find", Comm of the ACM, 31, 1192 (1988), doi:10.1145/63039.63042
// uses 32-bit int as seed
// requires no state, other than seed
// this interface could be extended as needed:
//   function to return a random int from 1-N
//   functions for RNs from non-uniform distributions

#ifdef __cplusplus
extern "C" {
#endif


// Useful constants describing the behavior of the generator.
// New random number generators should provide these constants as well.
// These constants were based on the results of tests in
// metaproc/test/rng/test_rng.

// The probability with which exactly zero (double 0.0) values are produced.
#define RANDOM_PARK_ZERO_PROBABILITY 0.
     // random_park produces no zero values

// The smallest non-zero value produced
#define RANDOM_PARK_ZERO_EPS 4.6566128730773925781250e-10
     // 1/IM

// A small value above which the cdf (cumulative distribution function) of the
// rng is safe from quanization effects.
// I.e. a small value p such that for all q>p the probability of the rng
// generating a value less than or equal to q is very close to q.
#define RANDOM_PARK_ZERO_THRESHOLD 1.0E-8

// The probability with which exactly one (double 1.0) values are produced.
#define RANDOM_PARK_ONE_PROBABILITY 0.
     // random_park produces no one values

// The largest non-one value produced
#define RANDOM_PARK_ONE_EPS_M1 4.6566128730773925781250e-10
#define RANDOM_PARK_ONE_EPS (1. - 4.6566128730773925781250e-10)
     // (IM-1)/IM 
      
// inverse THRESHOLD. trusted near-one values.
// I.e. a big value p such that for all q<p the probability of the rng
// generating a value greater than or equal to q is very close to 1-q.
#define RANDOM_PARK_ONE_THRESHOLD_M1 1.0E-8
#define RANDOM_PARK_ONE_THRESHOLD (1. - 1.0E-8)

// function declarations

static inline double random_park(int *pseed);

// IM is 2^31 - 1
#define IA 16807
#define IM 2147483647
#define AM (1.0/IM)
#define IQ 127773
#define IR 2836

// generate a single RN, uniformly distributed from 0-1
// input seed = positive 32-bit signed int
// seed = 0 is a bad input
// seed is updated
// RN is returned

static inline int random_park_int(int *pseed)
{
  int seed = *pseed;
  int k = seed/IQ;
  seed = IA*(seed-k*IQ) - IR*k;
  if (seed < 0) seed += IM;
  *pseed = seed;
  return seed;
}

static inline double random_park(int *pseed)
{
  int seed = *pseed;
  int k = seed/IQ;
  seed = IA*(seed-k*IQ) - IR*k;
  if (seed < 0) seed += IM;
  *pseed = seed;
  return AM*seed;
}

#undef IA
#undef IM
#undef AM
#undef IQ
#undef IR

#ifdef __cplusplus
}
#endif
#endif
