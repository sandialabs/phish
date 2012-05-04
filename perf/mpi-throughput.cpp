// MPI-only one-way test for throughput
// Syntax: oneway N M S
//         N = # of messages
//         M = size of each message
//         S = do a safe send every this many messages (0 = never)

#include "mpi.h"
#include "stdlib.h"
#include "stdio.h"
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

  if (narg != 4) {
    if (me == 0) printf("Oneway syntax: oneway N M S\n");
    MPI_Finalize();
    exit(1);
  }

  int n = atoi(args[1]);
  int m = atoi(args[2]);
  int s = atoi(args[3]);

  char *buf = new char[m];
  MPI_Status status;

  // sending processor
  // sends N messages, waits for signal from receiver
  // prints timing

  if (me == 0) {
    for (int i = 0; i < m; i++) buf[i] = '\0';

    double time_start = MPI_Wtime();

    for (int i = 0; i < n; i++) {
      if (s && i % s == 0) MPI_Ssend(buf,m,MPI_BYTE,1,0,MPI_COMM_WORLD);
      else MPI_Send(buf,m,MPI_BYTE,1,0,MPI_COMM_WORLD);
    }

    MPI_Recv(buf,0,MPI_BYTE,1,0,MPI_COMM_WORLD,&status);

    const double elapsed = MPI_Wtime() - time_start;
    const double throughput = n / elapsed;
    const double megabits = (throughput * m * 8.0) / 1000000.0;
    std::cout << elapsed << "," << m << "," << n << "," << throughput << "," << megabits << "\n";

  // receiving processor
  // receives N messages
  // signals sender when done

  } else {
    for (int i = 0; i < n; i++)
      MPI_Recv(buf,m,MPI_BYTE,0,0,MPI_COMM_WORLD,&status);

    MPI_Send(buf,0,MPI_BYTE,0,0,MPI_COMM_WORLD);
  }

  // clean up

  delete [] buf;
  MPI_Finalize();
}
