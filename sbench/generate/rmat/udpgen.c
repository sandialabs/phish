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
#include "rmat.h"

udp_throw_t * udpt = NULL;

rmat_t * rmat = NULL;

int max_packets = 10000000;
int events_per_pkt = 50;
int rate = 0;

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

     while ( (op = getopt(argc, argv, "I:R:b:r:h:")) != EOF) {
          switch (op) {
          case 'I':
               max_packets = atoi(optarg);
               break;
          case 'R':
               rmat = rmat_init_str(optarg);
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

     if (!rmat) {
          rmat = rmat_init(32, 0.5, 0.3, 0.15, 0.05);
     }

     if (!udpt->clientcnt) {
          fprintf(stderr, "no clients\n");
          return -1;
     }

     int buflen = 64*events_per_pkt;
     char *buf = (char *) malloc(buflen*sizeof(char));
     memset(buf,0,buflen);

     int i, j;

     uint64_t x = 0;
     uint64_t y = 0;

     fprintf(stderr, "starting generator\n");
     double timestart = myclock();

     for (i = 0; i < max_packets; i++) {
          int offset = snprintf(buf, buflen, "frame %d\n", i);
          
          for (j = 0; j < events_per_pkt; j++) {
               rmat_next_edge(rmat, &x, &y);
               offset += snprintf(buf+offset, buflen - offset,
                                  "%" PRIu64 ",%" PRIu64 "\n", x, y);
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

     double timestop = myclock();
     printf("Generation rate = %g events/sec\n",
            max_packets*events_per_pkt/(timestop-timestart));

     return 0;
}
