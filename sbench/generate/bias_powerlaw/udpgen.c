#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <inttypes.h>
#include "udp_throw.h"
#include "powerlaw.h"
#include "evahash.h"

udp_throw_t * udpt = NULL;

int max_packets = 10000000;
int events_per_pkt = 50;
int rate = 0;

unsigned int seed = 0;

#define DEFAULT_PORT 5555
#define BUFLEN 1400

int init_peer(char * peer) {
     uint16_t port = DEFAULT_PORT;
     int ret = 0;
     char * hostname = NULL;

     char * sep = strchr(peer, '@');
     if (!sep) {
          hostname = strdup(peer);
     }
     else {
          int len = sep - peer;
          hostname = strndup(peer, len);
          port = atoi(sep + 1);
     }

     if (hostname) {
          ret = udp_throw_add_client(udpt, hostname, port);

          free(hostname);
     }

     return ret;
}

int read_cmd_options(int argc, char ** argv) {
     register int op;

     while ( (op = getopt(argc, argv, "s:I:b:r:h:")) != EOF) {
          switch (op) {
          case 's':
               seed = atoi(optarg);
               break;
          case 'I':
               max_packets = atoi(optarg);
               break;
          case 'b':
               events_per_pkt = atoi(optarg);
               break;
          case 'r':
               rate = atoi(optarg);
               break;
          case 'h':
               init_peer(optarg);  
               break;
          }
     }
     while(optind < argc) {
          init_peer(argv[optind]);
          optind++;
     }

     return 1;
}

double myclock() {
     double time;
     struct timeval tv;

     gettimeofday(&tv,NULL);
     time = 1.0 * tv.tv_sec + 1.0e-6 * tv.tv_usec;
     return time;
}

int main(int argc, char ** argv) {
     udpt = udp_throw_init(0);

     if (!udpt) {
          fprintf(stderr, "unable to generate data\n");
          return -1;
     }
    
     read_cmd_options(argc, argv);

     if (!seed) {
          seed = (unsigned int) time(NULL);
          srand(seed);
     }

     double concentration = 0.5;
     unsigned long max_keys = 100000;
     unsigned long max_random = RAND_MAX;
     uint64_t keyoffset = rand();
     uint32_t eseed = rand();

     fprintf(stderr, "initalizing generator\n");
     power_law_distribution_t * power = 
       power_law_initialize(concentration, max_keys,max_random);

     if (!udpt->clientcnt) {
          fprintf(stderr, "no clients\n");
          return -1;
     }

     int buflen = 64*events_per_pkt;
     char *buf = (char *) malloc(buflen*sizeof(char));
     memset(buf,0,buflen);

     int i, j;

     fprintf(stderr, "starting generator\n");
     double timestart = myclock();

     for (i = 0; i < max_packets; i++) {
          int offset = snprintf(buf, buflen, "frame %d\n", i);
          
          for (j = 0; j < events_per_pkt; j++) {
               uint64_t r = rand_r(&seed);

               uint64_t key = power_law_simulate(r, power) + keyoffset;
               uint32_t val = 0;
               uint32_t bias = 0;
               if ((evahash((uint8_t*)&key,
                            sizeof(uint64_t), eseed) & 0xFF) == 0x11) {
                    bias = 1;
                    val = ((rand_r(&seed) & 0xF) == 0);
               }
               else {
                    val = rand_r(&seed) & 0x1;
               }
               offset += snprintf(buf+offset, buflen - offset,
                                  "%" PRIu64 ",%u,%u\n", key, val, bias);
          }

          udp_throw_data(udpt, buf, offset);

          if (rate) {
            double n = 1.0*(i+1)*events_per_pkt;
            double elapsed = myclock() - timestart;
            double actual_rate = n/elapsed;
            if (actual_rate > rate) {
              double delay = n/rate - elapsed;
              usleep(1.0e6*delay);
            }
          }
     }

     udp_throw_data(udpt, buf, 0);

     double timestop = myclock();
     printf("Generation rate = %g events/sec\n",
            max_packets*events_per_pkt/(timestop-timestart));

     power_law_destroy(power);

     return 0;
}
