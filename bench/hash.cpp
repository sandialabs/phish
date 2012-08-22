// MPI-only hash
// generate hashed messages on senders, send to receivers
// half of procs are senders, half are receivers

// Syntax: hash N M S
//         N = # of messages for all senders
//         M = size of each message
//         S = do a safe send every this many messages (0 = never)

#include "mpi.h"
#include "stdlib.h"
#include "stdio.h"
#include "hashlittle.h"

/* ---------------------------------------------------------------------- */

int main(int narg, char **args)
{
  MPI_Init(&narg,&args);

  int me,nprocs;
  MPI_Comm_rank(MPI_COMM_WORLD,&me);
  MPI_Comm_size(MPI_COMM_WORLD,&nprocs);

  // must run on even # of procs

  if (nprocs % 2) {
    if (me == 0) printf("Must run on even # of processors\n");
    MPI_Finalize();
    exit(1);
  }

  // parse input args

  if (narg != 4) {
    if (me == 0) printf("Hash syntax: hash N M S\n");
    MPI_Finalize();
    exit(1);
  }

  int n = atoi(args[1]);
  int m = atoi(args[2]);
  int s = atoi(args[3]);

  int nsource = nprocs/2;
  int nme = n / nsource;
  if (me < n % nsource) nme++;

  char *buf = new char[m];
  for (int i = 0; i < m; i++) buf[i] = '\0';
  srand48(12345);
  double rn;

  int proc,count;
  MPI_Status status;

  int host_length;
  char host[MPI_MAX_PROCESSOR_NAME];
  MPI_Get_processor_name(host,&host_length);
  host[host_length] = '\0';
  if (me < nsource) printf("PHISH host source %d: %s\n",me,host);
  else printf("PHISH host reduce %d: %s\n",me,host);

  // start timer on all procs

  MPI_Barrier(MPI_COMM_WORLD);
  double time_start = MPI_Wtime();

  // sending processor
  // sends Nme messages to randomly hashed receivers
  // when done, send tag=1 message to every receiver

  if (me < nsource) {
    for (int i = 0; i < nme; i++) {
      rn = drand48();
      proc = 
        hashlittle((char *) &rn,sizeof(double),nsource) % nsource + nsource;
      if (s && i % s == 0) MPI_Ssend(buf,m,MPI_BYTE,proc,0,MPI_COMM_WORLD);
      else MPI_Send(buf,m,MPI_BYTE,proc,0,MPI_COMM_WORLD);
    }

    for (proc = nsource; proc < nprocs; proc++)
      MPI_Send(buf,m,MPI_BYTE,proc,1,MPI_COMM_WORLD);

  // receiving processor
  // receive messages from any sender
  // done when receive tag=1 message from every sender

  } else {
    int stop = 0;
    count = 0;

    while (1) {
      MPI_Recv(buf,m,MPI_BYTE,MPI_ANY_SOURCE,MPI_ANY_TAG,
	       MPI_COMM_WORLD,&status);
      if (status.MPI_TAG == 1) stop++;
      else count++;
      if (stop == nsource) break;
    }
  }

  // stop timer on all procs

  MPI_Barrier(MPI_COMM_WORLD);
  double time_stop = MPI_Wtime();
  if (me == 0)
    printf("Elapsed time for %d hashed messages of %d bytes "
           "from %d sources = %g secs\n",
	   n,m,nsource,time_stop-time_start);

  // clean up

  delete [] buf;
  MPI_Finalize();
}
