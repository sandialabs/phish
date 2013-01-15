#ifndef _RMAT_H
#define _RMAT_H 

#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "random_park.h"

#ifdef __cplusplus
extern "C" {
#endif

#define EPSILON 1.0e-6

typedef struct _rmat_t {
     uint64_t iterations;
     int nlevels;
     double a;
     double b;
     double c; 
     double d;
     int seed;
     double fraction;
} rmat_t;

static rmat_t * rmat_init(int levels, double a, double b, double c, double d) {
     rmat_t * r = (rmat_t*)calloc(1, sizeof(rmat_t));
     if (!r) {
          return NULL;
     }
     r->seed = 12345;
     r->nlevels = levels;
     r->a = a;
     r->b = b;
     r->c = c;
     r->d = d;

     if (r->nlevels < 0 || r->nlevels >= 64) {
          fprintf(stderr,"RMAT matrix size is invalid\n");
          return NULL;
     }

     if (r->a < 0.0 || r->b < 0.0 || r->c < 0.0 || r->d < 0.0) {
          fprintf(stderr, "a,b,c,d params are invalid\n");
          return NULL;
     }

     if (fabs(r->a + r->b + r->c + r->d - 1.0) > EPSILON) {
          fprintf(stderr,"a,b,c,d params must sum to 1\n");
          return NULL;
     }

     r->d = 1.0 - (r->a + r->b + r->c);

     return r;
}

// process command line args
static inline rmat_t * rmat_init_arg(int argc, char **argv) {

     // required args
     if (argc != 5) {
          return NULL;
     }

     return rmat_init(atoi(argv[0]), atof(argv[1]), atof(argv[2]), atof(argv[3]),
               atof(argv[4]));

}

static rmat_t * rmat_init_str(char * buf) {

     int levels = 0;
     double a = 0;
     double b = 0;
     double c = 0;
     double d = 0;

     int res = sscanf(buf, "%d %lf %lf %lf %lf", &levels, &a, &b, &c, &d);

     if (res != 5) {
          fprintf(stderr, "invalid rmat parameters");
          return NULL;
     }

     return rmat_init(levels, a, b, c, d);
}

static inline void rmat_seed(rmat_t * r, int seed) {
     if (!r) {
          return;
     }
     r->seed = seed;
}

//return 1 on success
static inline int rmat_next_edge(rmat_t * r, uint64_t * v1, uint64_t *v2) {
     if (!r || !v1 || !v2) {
          return 0;
     }

     double a,b,c,d,fraction;
     double a1,b1,c1,d1,total,rn;
     int ilevel;

     r->iterations++;

     // generate a single (I,J) edge from RMAT distribution

     a = r->a;
     b = r->b;
     c = r->c;
     d = r->d;
     fraction = r->fraction;
     int nlevels = r->nlevels;

     uint64_t norder = ((uint64_t) 1) << nlevels;
     uint64_t delta = norder >> 1;

     a1 = a; b1 = b; c1 = c; d1 = d;
     uint64_t i = 0;
     uint64_t j = 0;

     for (ilevel = 0; ilevel < nlevels; ilevel++) {
          rn = random_park(&r->seed);
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
               a1 += a1*fraction * (random_park(&r->seed) - 0.5);
               b1 += b1*fraction * (random_park(&r->seed) - 0.5);
               c1 += c1*fraction * (random_park(&r->seed) - 0.5);
               d1 += d1*fraction * (random_park(&r->seed) - 0.5);
               total = a1+b1+c1+d1;
               a1 /= total;
               b1 /= total;
               c1 /= total;
               d1 /= total;
          }
     }
     *v1 = i;
     *v2 = j;

     return 1;
}

// return 1 if successful
// return 0 if not

static inline int rmat_destroy(rmat_t * r) {
     if (r) {
          free(r);
     }
     return 1;
}

#ifdef __cplusplus
}
#endif

#endif
