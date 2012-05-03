// MPI-only loop test
// Syntax: loop N M S
//         N = # of messages
//         M = size of each message
//         S = do a safe send every this many messages (0 = never)

#include "mpi.h"
#include "stdlib.h"
#include "stdio.h"

/* ---------------------------------------------------------------------- */

int main(int narg, char **args)
{
  MPI_Init(&narg,&args);

  int me,nprocs;
  MPI_Comm_rank(MPI_COMM_WORLD,&me);
  MPI_Comm_size(MPI_COMM_WORLD,&nprocs);

  // must run on >= 2 procs

  if (nprocs < 2) {
    if (me == 0) printf("Must run on 2 or more processors\n");
    MPI_Finalize();
    exit(1);
  }

  // parse input args

  if (narg != 4) {
    if (me == 0) printf("Loop syntax: loop N M S\n");
    MPI_Finalize();
    exit(1);
  }

  int n = atoi(args[1]);
  int m = atoi(args[2]);
  int s = atoi(args[3]);

  char *buf = new char[m];
  MPI_Status status;

  // head processor
  // sends N messages to next proc, waits for signal from tail
  // prints timing

  if (me == 0) {
    for (int i = 0; i < m; i++) buf[i] = '\0';

    double time_start = MPI_Wtime();

    for (int i = 0; i < n; i++) {
      if (s && i % s == 0) MPI_Ssend(buf,m,MPI_BYTE,1,0,MPI_COMM_WORLD);
      else MPI_Send(buf,m,MPI_BYTE,1,0,MPI_COMM_WORLD);
    }

    MPI_Recv(buf,0,MPI_BYTE,nprocs-1,0,MPI_COMM_WORLD,&status);

    double time_stop = MPI_Wtime();
    printf("Elapsed time for %d loop messages of %d bytes = %g secs\n",
	   n,m,time_stop-time_start);

  // in-between processor
  // receives N messages from prev proc
  // sends N messages to next proc

  } else if (me < nprocs-1) {
    for (int i = 0; i < n; i++) {
      MPI_Recv(buf,m,MPI_BYTE,me-1,0,MPI_COMM_WORLD,&status);
      if (s && i % s == 0) MPI_Ssend(buf,m,MPI_BYTE,me+1,0,MPI_COMM_WORLD);
      else MPI_Send(buf,m,MPI_BYTE,me+1,0,MPI_COMM_WORLD);
    }

  // tail processor
  // receives N messages from prev proc
  // signals head when done

  } else {
    for (int i = 0; i < n; i++)
      MPI_Recv(buf,m,MPI_BYTE,me-1,0,MPI_COMM_WORLD,&status);
    
    MPI_Send(buf,0,MPI_BYTE,0,0,MPI_COMM_WORLD);
  }

  // clean up

  delete [] buf;
  MPI_Finalize();
}
