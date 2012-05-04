// MPI-only ping-pong test for latency
// Syntax: pingpong N M
//         N = # of messages
//         M = size of each message

#include "mpi.h"
//#include "stdlib.h"
//#include "stdio.h"
#include <iostream>

/* ---------------------------------------------------------------------- */

int main(int narg, char **args)
{
  MPI_Init(&narg,&args);

  int me,nprocs;
  MPI_Comm_rank(MPI_COMM_WORLD,&me);
  MPI_Comm_size(MPI_COMM_WORLD,&nprocs);

  // must run on 2 procs

  if (nprocs != 2) {
    if (me == 0) printf("Must run on 2 processors\n");
    MPI_Finalize();
    exit(1);
  }

  // parse input args

  if (narg != 3) {
    if (me == 0) printf("Pingpong syntax: pingpong N M\n");
    MPI_Finalize();
    exit(1);
  }

  int n = atoi(args[1]);
  int m = atoi(args[2]);

  char *buf = new char[m];
  MPI_Status status;

  // ping processor
  // sends and receives
  // prints timing

  if (me == 0) {
    for (int i = 0; i < m; i++) buf[i] = '\0';

    double time_start = MPI_Wtime();

    for (int i = 0; i < n; i++) {
      MPI_Send(buf,m,MPI_BYTE,1,0,MPI_COMM_WORLD);
      MPI_Recv(buf,m,MPI_BYTE,1,0,MPI_COMM_WORLD,&status);
    }

    double elapsed = MPI_Wtime() - time_start;
    const double latency = elapsed / (n * 2.0);

    std::cout << elapsed << "," << m << "," << n << "," << (latency * 1000000.0) << "\n";

  // pong processor
  // receives and sends

  } else {
    for (int i = 0; i < n; i++) {
      MPI_Recv(buf,m,MPI_BYTE,0,0,MPI_COMM_WORLD,&status);
      MPI_Send(buf,m,MPI_BYTE,0,0,MPI_COMM_WORLD);
    }
  }

  // clean up

  delete [] buf;
  MPI_Finalize();
}
