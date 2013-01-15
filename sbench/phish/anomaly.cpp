// MINNOW anomaly
// print found anomalies to screen

#include "stdlib.h"
#include "string.h"
#include "stdio.h"
#include "phish.h"

#include <tr1/unordered_map>

#include <signal.h>
#include <netinet/in.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/types.h>

void frame(int);
void stats();

/* ---------------------------------------------------------------------- */

int perframe = 50;

int anomaly = 0;
int correct = 0;
int pfalse = 0;
int nfalse = 0;

// thresh = N,M
// flag as anomaly if key appears N times with value=1 <= M times
// thresh = 12,2 flags many false positives

int nthresh = 24;
int mthresh = 4;

std::tr1::unordered_map<uint64_t, std::pair<int,int> > kv;

/* ---------------------------------------------------------------------- */

int main(int narg, char **args)
{
  phish_init(&narg,&args);
  phish_input(0,frame,NULL,1);
  phish_check();

  phish_loop();
  phish_exit();
}

/* ---------------------------------------------------------------------- */

void frame(int nvalues)
{
  char *buf;
  int len;
  phish_unpack(&buf,&len);

  char *p = strtok(&buf[0],"\n");
  for (int i = 0; i < perframe; i++) {
    int64_t key = atol(strtok(NULL,",\n"));
    int64_t value = atoi(strtok(NULL,",\n"));
    int64_t truth = atoi(strtok(NULL,",\n"));

    // store datum in hash table
    // value = (N,M)
    // N = # of times key has been seen
    // M = # of times flag = 1
    // M < 0 means already seen N times
    
    if (!kv.count(key)) kv[key] = std::pair<int,int>(1,value);
    else {
      std::pair<int,int>& tuple = kv[key];
      if (tuple.second < 0) {
        ++tuple.first;
        continue;
      }

      ++tuple.first;
      tuple.second += value;
      if (tuple.first >= nthresh) {
        if (tuple.second > mthresh) {
          if (truth) {
            nfalse++;
            printf("FALSE NEG %ld %d\n",key,truth);
          }
        } else {
          anomaly++;
          if (truth) {
            correct++;
            printf("ANOMALY %ld %d\n",key,truth);
          } else {
            pfalse++;
            printf("FALSE POS %ld %d\n",key,truth);
          }
        }
        tuple.second = -1;
      }
    }
  }
}
