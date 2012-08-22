// MPI-only self test of sending messages to yourself
// Syntax: self N M S
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

  // must run on 1 proc

  if (nprocs != 1) {
    if (me == 0) printf("Must run on 1 processor\n");
    MPI_Finalize();
    exit(1);
  }

  // parse input args

  if (narg != 4) {
    if (me == 0) printf("Self syntax: self N M S\n");
    MPI_Finalize();
    exit(1);
  }

  int n = atoi(args[1]);
  int m = atoi(args[2]);
  int s = atoi(args[3]);

  char *mpibuf = new char[m+1000];
  MPI_Buffer_attach(mpibuf,m+1000);

  char *buf = new char[m];
  for (int i = 0; i < m; i++) buf[i] = '\0';

  MPI_Status status;

  // send a message, receive it

  double time_start = MPI_Wtime();

  for (int i = 0; i < n; i++) {
    printf("PRESEND %d: %d %d\n",me,s,m);
    MPI_Bsend(buf,m,MPI_BYTE,0,0,MPI_COMM_WORLD);
    printf("POSTSEND %d: %d %d\n",me,s,m);
    MPI_Recv(buf,m,MPI_BYTE,0,0,MPI_COMM_WORLD,&status);
  }

  double time_stop = MPI_Wtime();
  printf("Elapsed time for %d self messages of %d bytes = %g secs\n",
         n,m,time_stop-time_start);

  // clean up

  delete [] mpibuf;
  delete [] buf;
  MPI_Finalize();
}
