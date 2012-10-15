// MINNOW edge
// receive and store a graph edge
// mode = 0, don't store at all
// mode = 1, store once as (Vi,Vj)
// mode = 2, store once as (Vi,Vj), send to Vj owner, store again as (Vj,Vi)

#include "stdlib.h"
#include "stdio.h"
#include "phish.h"

#include <tr1/unordered_map>
#include <tr1/unordered_set>

int nedge;
std::tr1::unordered_map<uint64_t, std::tr1::unordered_set<uint64_t> > graph;

void edge_once(int);
void edge_first(int);
void edge_second(int);
void edge_close();

/* ---------------------------------------------------------------------- */

int main(int narg, char **args)
{
  phish_init(&narg,&args);

  if (narg != 2) phish_error("Edge syntax: edge 0/1/2");
  int mode = atoi(args[1]);
  if (mode < 0 or mode > 2) phish_error("Edge syntax: edge 0/1/2");

  if (mode == 0) {
    phish_input(0,NULL,edge_close,0);
    phish_input(1,NULL,NULL,0);
  } else if (mode == 1) {
    phish_input(0,edge_once,edge_close,1);
    phish_input(1,NULL,NULL,0);
  } else if (mode == 2) {
    phish_input(0,edge_first,edge_close,1);
    phish_input(1,edge_second,NULL,1);
  }

  phish_output(0);
  phish_output(1);
  phish_check();

  nedge = 0;

  int nlocal = phish_query("nlocal",0,0);
  double time_start = phish_timer();

  phish_loop();

  double time_stop = phish_timer();
  printf("Elapsed time for edge on %d procs = %g secs, stored %d edges",
         nlocal,time_stop-time_start,nedge);

  phish_exit();
}

/* ---------------------------------------------------------------------- */

// process 1st copy of edge = (Vi,Vj)
// sent by edge source to owner of Vi
// ignore self edges and duplicate edges
// store edge

void edge_once(int nvalues)
{
  char *buf;
  int type,tmp;
  type = phish_unpack(&buf,&tmp);
  uint64_t vi = *((uint64_t *) buf);
  type = phish_unpack(&buf,&tmp);
  uint64_t vj = *((uint64_t *) buf);
  if (vi == vj) return;

  if (!graph.count(vi))
    graph[vi] = std::tr1::unordered_set<uint64_t>();
  if (!graph[vi].count(vj)) {
    graph[vi].insert(vj);
    ++nedge;
  }
}

// process 1st copy of edge = (Vi,Vj)
// sent by edge source to owner of Vi
// ignore self edges and duplicate edges
// store edge
// send (Vj,Vi) to owner of Vj on port 1

void edge_first(int nvalues)
{
  char *buf;
  int type,tmp;
  type = phish_unpack(&buf,&tmp);
  uint64_t vi = *((uint64_t *) buf);
  type = phish_unpack(&buf,&tmp);
  uint64_t vj = *((uint64_t *) buf);
  if (vi == vj) return;

  if (!graph.count(vi))
    graph[vi] = std::tr1::unordered_set<uint64_t>();
  if (!graph[vi].count(vj)) {
    graph[vi].insert(vj);
    ++nedge;
    phish_pack_uint64(vj);
    phish_pack_uint64(vi);
    phish_send_key(1,(char *) &vj,sizeof(uint64_t));
  }
}

// process 2nd copy of edge = (Vi,Vj)
// sent by another edge minnow in edge_first() to owner of Vi
// store edge unless duplicate

void edge_second(int nvalues)
{
  char *buf;
  int type,tmp;
  type = phish_unpack(&buf,&tmp);
  uint64_t vi = *((uint64_t *) buf);
  type = phish_unpack(&buf,&tmp);
  uint64_t vj = *((uint64_t *) buf);

  if (!graph.count(vi))
    graph[vi] = std::tr1::unordered_set<uint64_t>();
  if (!graph[vi].count(vj)) {
    graph[vi].insert(vj);
    ++nedge;
  }
}

// close port to self

void edge_close()
{
  phish_close(1);
}
