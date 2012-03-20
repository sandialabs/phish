/* ----------------------------------------------------------------------
   MR-MPI = MapReduce-MPI library
   http://www.cs.sandia.gov/~sjplimp/mapreduce.html
   Steve Plimpton, sjplimp@sandia.gov, Sandia National Laboratories

   Copyright (2009) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under 
   the modified Berkeley Software Distribution (BSD) License.

   See the README file in the top-level MapReduce directory.
------------------------------------------------------------------------- */

// MPI version of PHISH library

#include "mpi.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "stdint.h"
#include "phish.h"
#include "hash.h"

// ZMQ support for publish/subscribe connections

#ifdef PHISH_MPI_ZMQ
#include "zmq.h"
#endif

/* ---------------------------------------------------------------------- */
// definitions

// connection styles

enum{SINGLE,PAIRED,HASHED,ROUNDROBIN,DIRECT,BCAST,CHAIN,RING,PUBLISH,SUBSCRIBE};

// port status

enum{UNUSED_PORT,OPEN_PORT,CLOSED_PORT};

#define KBYTE 1024       // 1 Kbyte
#define MAXPORT 16       // max # of input and output ports
                         // hardwired to insure all minnows use same value

#define DELTAQUEUE 16    // increment for # queue entries

typedef void (DatumFunc)(int);         // callback prototypes
typedef void (DoneFunc)();
typedef void (AbortFunc)(int);

/* ---------------------------------------------------------------------- */
// variables local to single PHISH instance

MPI_Comm world;           // MPI communicator
int me,nprocs;            // MPI rank and total # of procs

int initflag;             // 1 if phish_init has been called
int checkflag;            // 1 if phish_check has been called

char *exename;            // name of minnow executable
char *idminnow;           // ID of minnow in input script
int idlocal;              // index of minnow within its set
int nlocal;               // # of duplicate minnows via layout command
int idglobal;             // index of minnow within global school
int nglobal;              // # of total minnows in global school

int maxbuf;               // max allowed datum size in bytes
int memchunk;             // max allowed datum size in Kbytes
int safe;                 // 1 for safe MPI send, 0 for normal

DoneFunc *alldonefunc;    // callback when all input ports closed
AbortFunc *abortfunc;     // callback before aborting

// input ports
// each can have multiple connections from output ports of other minnows

struct InConnect {        // inbound connect from output port of other minnow
  int style;              // SINGLE, HASHED, etc
  int nsend;              // # of procs that send to me on this connection
  char *host;             // hostname:port for SUBSCRIBE input
};

struct InputPort {        // one input port
  int status;             // UNUSED or OPEN or CLOSED
  int donecount;          // # of done messages received on this port
  int donemax;            // # of done messages that will close this port
  int nconnect;           // # of connections to this port
  InConnect *connects;    // list of connections
  DatumFunc *datumfunc;   // callback when receive datum on this port
  DoneFunc *donefunc;     // callback when this port closes
};

InputPort *inports;       // list of input ports
int ninports;             // # of used input ports
int donecount;            // # of closed input ports
int lastport;             // last input port a datum was received on

// output ports
// each can have multiple connections to input ports of other minnows

struct OutConnect {        // outbound connect to input port of other minnow
  int style;               // SINGLE, HASHED, etc
  int nrecv;               // # of procs that receive from me
  int recvone;             // single proc ID I send to (nrecv = 1)
  int recvfirst;           // 1st proc ID I send to (nrecv > 1)
  int offset;              // offset from 1st proc for roundrobin
  int recvport;            // port to send to on receivers
  int tcpport;             // port for PUBLISH output
};

struct OutPort {           // one output port
  int status;              // UNUSED or OPEN or CLOSED
  int nconnect;            // # of connections from this port
  OutConnect *connects;    // list of connections
};

OutPort *outports;         // list of output ports
int noutports;             // # of used output ports

// send/receive buffers that hold a datum

char *sbuf;                // buffer to hold datum to send
int nsbytes;               // total size of send datum
char *sptr;                // ptr to current loc in sbuf for packing
int npack;                 // # of fields packed thus far into sbuf

char *rbuf;                // buffer to hold received datum
int nrbytes;               // total size of received datum
int nrfields;              // # of fields in received datum
char *rptr;                // ptr to current loc in rbuf for unpacking
int nunpack;               // # of fields unpacked thus far from rbuf

// queued datums

struct Queue {             // one queued datum
  char *datum;             // copy of entire datum
  int nbytes;              // size of datum
  int iport;               // port the datum was received on
};

Queue *queue;              // internal queue
int nqueue;                // # of datums in queue
int maxqueue;              // max # of datums queue can hold

// stats

uint64_t rcount;           // # of datums received
uint64_t scount;           // # of datums sent

// internal function prototypes

void send(OutConnect *);
void stats();
void allocate_ports();
void deallocate_ports();

/* ---------------------------------------------------------------------- */

void phish_init(int *pnarg, char ***pargs)
{
  if (initflag) phish_error("Phish_init has already been called");

  // initialize MPI

  MPI_Init(pnarg,pargs);

  world = MPI_COMM_WORLD;
  MPI_Comm_rank(world,&me);
  MPI_Comm_size(world,&nprocs);

  // library status

  initflag = 1;
  checkflag = 0;
  lastport = -1;
  nqueue = maxqueue = 0;
  queue = NULL;

  // default settings

  memchunk = 1;
  safe = 0;
  alldonefunc = NULL;
  abortfunc = NULL;

  // port allocation

  allocate_ports();

  // parse input args and setup communication port data structs

  char **args = *pargs;
  int narg = *pnarg;
  int argstart = narg;
  
  int iarg = 1;
  while (iarg < narg) {
    if (strcmp(args[iarg],"-minnow") == 0) {
      if (iarg+5 > narg)
	phish_error("Invalid command-line args in phish_init");

      int n = strlen(args[iarg+1]) + 1;
      exename = new char[n];
      strcpy(exename,args[iarg+1]);
      n = strlen(args[iarg+2]) + 1;
      idminnow = new char[n];
      strcpy(idminnow,args[iarg+2]);
      nlocal = atoi(args[iarg+3]);
      int nprev = atoi(args[iarg+4]);
      idlocal = me - nprev;
      idglobal = me;
      nglobal = nprocs;
      iarg += 5;

    } else if (strcmp(args[iarg],"-memory") == 0) {
      if (iarg+2 > narg)
	phish_error("Invalid command-line args in phish_init");
      memchunk = atoi(args[iarg+1]);
      if (memchunk < 0) 
	phish_error("Invalid command-line args in phish_init");

    } else if (strcmp(args[iarg],"-safe") == 0) {
      safe = 1;

    } else if (strcmp(args[iarg],"-in") == 0) {
      if (iarg+8 > narg)
	phish_error("Invalid command-line args in phish_init");

      int style;
      int sprocs,sfirst,sport,rprocs,rfirst,rport;
      char *host = NULL;

      sprocs = atoi(args[iarg+1]);
      sfirst = atoi(args[iarg+2]);
      sport = atoi(args[iarg+3]);

      if (strcmp(args[iarg+4],"single") == 0) style = SINGLE;
      else if (strcmp(args[iarg+4],"paired") == 0) style = PAIRED;
      else if (strcmp(args[iarg+4],"hashed") == 0) style = HASHED;
      else if (strcmp(args[iarg+4],"roundrobin") == 0) style = ROUNDROBIN;
      else if (strcmp(args[iarg+4],"direct") == 0) style = DIRECT;
      else if (strcmp(args[iarg+4],"bcast") == 0) style = BCAST;
      else if (strcmp(args[iarg+4],"chain") == 0) style = CHAIN;
      else if (strcmp(args[iarg+4],"ring") == 0) style = RING;
#ifdef PHISH_MPI_ZMQ
      else if (strstr(args[iarg+4],"subscribe") == args[iarg+4]) {
	style = SUBSCRIBE;
	int n = strlen(args[iarg+4]) - strlen("subscribe/") + 1;
	host = new char[n];
	strcpy(host,&args[iarg+4][strlen("subscribe/")]);
      }
#endif
      else phish_error("Unrecognized in style in phish_init");

      rprocs = atoi(args[iarg+5]);
      rfirst = atoi(args[iarg+6]);
      rport = atoi(args[iarg+7]);

      if (rport > MAXPORT)
	phish_error("Invalid input port ID in phish_init");
      InputPort *ip = &inports[rport];
      ip->status = CLOSED_PORT;
      ip->nconnect++;
      ip->connects = (InConnect *) 
	realloc(ip->connects,ip->nconnect*sizeof(InConnect));
      InConnect *ic = &ip->connects[ip->nconnect-1];

      ic->style = style;
      ic->nsend = sprocs;
      ic->host = host;

      switch (style) {
      case SINGLE:
      case HASHED:
      case ROUNDROBIN:
      case DIRECT:
      case BCAST:
	ip->donemax += sprocs;
	break;
      case PAIRED:
      case CHAIN:
      case RING:
	ip->donemax++;
	break;
      case SUBSCRIBE:
	break;
      }

      iarg += 8;

    } else if (strcmp(args[iarg],"-out") == 0) {
      if (iarg+8 > narg) 
	phish_error("Invalid command-line args in phish_init");

      int style;
      int sprocs,sfirst,sport,rprocs,rfirst,rport;
      int tcpport = 0;

      sprocs = atoi(args[iarg+1]);
      sfirst = atoi(args[iarg+2]);
      sport = atoi(args[iarg+3]);

      if (strcmp(args[iarg+4],"single") == 0) style = SINGLE;
      else if (strcmp(args[iarg+4],"paired") == 0) style = PAIRED;
      else if (strcmp(args[iarg+4],"hashed") == 0) style = HASHED;
      else if (strcmp(args[iarg+4],"roundrobin") == 0) style = ROUNDROBIN;
      else if (strcmp(args[iarg+4],"direct") == 0) style = DIRECT;
      else if (strcmp(args[iarg+4],"bcast") == 0) style = BCAST;
      else if (strcmp(args[iarg+4],"chain") == 0) style = CHAIN;
      else if (strcmp(args[iarg+4],"ring") == 0) style = RING;
#ifdef PHISH_MPI_ZMQ
      else if (strstr(args[iarg+4],"publish") == args[iarg+4]) {
	style = PUBLISH;
	tcpport = atoi(&args[iarg+4][strlen("publish/")]);
      }
#endif
      else phish_error("Unrecognized out style in phish_init");

      rprocs = atoi(args[iarg+5]);
      rfirst = atoi(args[iarg+6]);
      rport = atoi(args[iarg+7]);

      if (sport > MAXPORT)
	phish_error("Invalid output port ID in phish_init");
      OutPort *op = &outports[sport];
      op->status = CLOSED_PORT;
      op->nconnect++;
      op->connects = (OutConnect *) 
	realloc(op->connects,op->nconnect*sizeof(OutConnect));
      OutConnect *oc = &op->connects[op->nconnect-1];

      oc->recvport = rport;
      oc->style = style;
      oc->tcpport = tcpport;

      switch (style) {
      case SINGLE:
	oc->nrecv = 1;
	oc->recvone = rfirst;
	oc->recvfirst = -1;
	oc->offset = -1;
	break;
      case PAIRED:
	oc->nrecv = 1;
	oc->recvone = rfirst + me - sfirst;
	oc->recvfirst = -1;
	oc->offset = -1;
	break;
      case HASHED:
	oc->nrecv = rprocs;
	oc->recvone = -1;
	oc->recvfirst = rfirst;
	oc->offset = -1;
	break;
      case ROUNDROBIN:
	oc->nrecv = rprocs;
	oc->recvone = -1;
	oc->recvfirst = rfirst;
	oc->offset = 0;
	break;
      case DIRECT:
	oc->nrecv = rprocs;
	oc->recvone = -1;
	oc->recvfirst = rfirst;
	oc->offset = 0;
	break;
      case BCAST:
	oc->nrecv = rprocs;
	oc->recvone = -1;
	oc->recvfirst = rfirst;
	oc->offset = 0;
	break;
      case CHAIN:
	oc->nrecv = 1;
	oc->recvone = me + 1;
	if (me-sfirst == sprocs-1) {
	  oc->nrecv = 0;
	  oc->recvone = -1;
	}
	oc->recvfirst = -1;
	oc->offset = -1;
	break;
      case RING:
	// set nrecv and recvfirst so can invoke reset_receiver()
	// otherwise would set nrecv = 1, recvfirst = -1
	oc->nrecv = rprocs;
	oc->recvone = me + 1;
	if (me-rfirst == rprocs-1) oc->recvone = rfirst;
	oc->recvfirst = rfirst;
	oc->offset = -1;
	break;
      case PUBLISH:
	oc->nrecv = -1;
	oc->recvone = -1;
	oc->recvfirst = -1;
	oc->offset = -1;
	break;
      }

      iarg += 8;

    } else if (strcmp(args[iarg],"-args") == 0) {
      argstart = iarg+1;
      iarg = narg;

    } else phish_error("Invalid command-line args in phish_init");
  }

  // memory allocation/initialization for datum buffers

  maxbuf = memchunk * KBYTE;
  sbuf = (char *) malloc(maxbuf*sizeof(char));
  rbuf = (char *) malloc(maxbuf*sizeof(char));

  if (!sbuf || !rbuf) phish_error("Malloc of datum buffers failed");

  // strip off PHISH args, leaving app args for app to use

  *pnarg = narg-argstart;
  if (*pnarg > 0) *pargs = &args[argstart];
  else *pargs = NULL;

  // set send buffer ptr for initial datum

  sptr = sbuf + sizeof(int);
  npack = 0;
}

/* ---------------------------------------------------------------------- */

int phish_init_python(int narg, char **args)
{
  int narg_start = narg;
  phish_init(&narg,&args);
  return narg_start-narg;
}

/* ---------------------------------------------------------------------- */

void phish_exit()
{
  if (!initflag) phish_error("Phish_init has not been called");
  if (!checkflag) phish_error("Phish_check has not been called");

  // generate stats

  stats();

  // warn if any input port is still open

  for (int i = 0; i < MAXPORT; i++)
    if (inports[i].status == OPEN_PORT)
      phish_warn("Exiting with input port still open");

  // close all output ports

  for (int i = 0; i < MAXPORT; i++) phish_close(i);

  // free PHISH memory

  free(sbuf);
  free(rbuf);
  for (int i = 0; i < nqueue; i++) delete [] queue[i].datum;
  free(queue);

  deallocate_ports();
  delete [] exename;
  delete [] idminnow;

  // shut-down MPI

  MPI_Finalize();
  initflag = checkflag = 0;
}

/* ----------------------------------------------------------------------
   setup single input port iport
   reqflag = 1 if port must be used by input script
------------------------------------------------------------------------- */

void phish_input(int iport, void (*datumfunc)(int), 
		 void (*donefunc)(), int reqflag)
{
  if (!initflag) phish_error("Phish_init has not been called");
  if (checkflag) phish_error("Phish_check has already been called");

  if (iport < 0 || iport > MAXPORT)
    phish_error("Invalid port ID in phish_input");

  if (reqflag && inports[iport].status == UNUSED_PORT)
    phish_error("Input script does not use a required input port");

  if (inports[iport].status == UNUSED_PORT) return;
  inports[iport].status = OPEN_PORT;
  inports[iport].datumfunc = datumfunc;
  inports[iport].donefunc = donefunc;
}

/* ----------------------------------------------------------------------
   setup single output port iport
   no reqflag setting, since script does not have to use the port
------------------------------------------------------------------------- */

void phish_output(int iport)
{
  if (!initflag) phish_error("Phish_init has not been called");
  if (checkflag) phish_error("Phish_check has already been called");

  if (iport < 0 || iport > MAXPORT)
    phish_error("Invalid port count in phish_output");

  if (outports[iport].status == UNUSED_PORT) return;
  outports[iport].status = OPEN_PORT;
}

/* ----------------------------------------------------------------------
   check consistency of input args with ports setup by phish input/output
------------------------------------------------------------------------- */

void phish_check()
{
  if (!initflag) phish_error("Phish_init has not been called");
  if (checkflag) phish_error("Phish_check has already been called");
  checkflag = 1;

  // args processed by phish_init() requested various input ports
  // flagged them as CLOSED, others as UNUSED
  // phish_input() reset CLOSED ports to OPEN
  // error if a port is CLOSED, since phish_input was not called
  // set ninports = # of used input ports
  // initialize donecount before datum exchanges begin

  ninports = 0;
  for (int i = 0; i < MAXPORT; i++) {
    if (inports[i].status == CLOSED_PORT)
      phish_error("Input script uses an undefined input port");
    if (inports[i].status == OPEN_PORT) ninports++;
  }
  donecount = 0;

  // args processed by phish_init() requested various output ports
  // flagged them as CLOSED, others as UNUSED
  // phish_output() reset CLOSED ports to OPEN
  // error if a port is CLOSED, since phish_output was not called
  // set noutports = # of used output ports

  noutports = 0;
  for (int i = 0; i < MAXPORT; i++) {
    if (outports[i].status == CLOSED_PORT)
      phish_error("Input script uses an undefined output port");
    if (outports[i].status == OPEN_PORT) noutports++;
  }

  // stats

  rcount = scount = 0;
}

/* ----------------------------------------------------------------------
   set callback functions
   alldonefunc = invoked when all input ports are closed
   abortfunc = invoked when PHISH aborts via phish_error()
   allow this one func to be called before phish_init() is called
------------------------------------------------------------------------- */

void phish_callback(void (*func1)(), void (*func2)(int))
{
  if (func1) alldonefunc = func1;
  if (func2) abortfunc = func2;
}

/* ----------------------------------------------------------------------
   close output port iport
------------------------------------------------------------------------- */

void phish_close(int iport)
{
  if (!checkflag) phish_error("Phish_check has not been called");

  if (iport < 0 || iport >= MAXPORT)
    phish_error("Invalid port ID for phish_close");
  OutPort *op = &outports[iport];
  if (op->status != OPEN_PORT) return;

  // loop over connections
  // loop over all receivers in connection
  // send done message to each proc in receiver that I send to
  // send message to appropriate input port of receiving proc

  for (int iconnect = 0; iconnect < op->nconnect; iconnect++) {
    OutConnect *oc = &op->connects[iconnect];
    int tag = MAXPORT + oc->recvport;
    switch (oc->style) {
    case SINGLE:
    case PAIRED:
    case RING:
      if (safe) MPI_Ssend(NULL,0,MPI_BYTE,oc->recvone,tag,world);
      else MPI_Send(NULL,0,MPI_BYTE,oc->recvone,tag,world);
      break;

    case HASHED:
    case ROUNDROBIN:
    case DIRECT:
    case BCAST:
      for (int i = 0; i < oc->nrecv; i++)
        if (safe) MPI_Ssend(NULL,0,MPI_BYTE,oc->recvfirst+i,tag,world);
        else MPI_Send(NULL,0,MPI_BYTE,oc->recvfirst+i,tag,world);
      break;

    case CHAIN:
      if (oc->nrecv) {
	if (safe) MPI_Ssend(NULL,0,MPI_BYTE,oc->recvone,tag,world);
	else MPI_Send(NULL,0,MPI_BYTE,oc->recvone,tag,world);
      }
      break;
    }
  }

  outports[iport].status = CLOSED_PORT;
}

/* ----------------------------------------------------------------------
   infinite loop on incoming datums
   blocking MPI_Recv() for a datum
   check datum for DONE message, else callback to datumfunc()
------------------------------------------------------------------------- */

void phish_loop()
{
  int iport,doneflag;
  MPI_Status status;

  if (!checkflag) phish_error("Phish_check has not been called");

  while (1) {
    MPI_Recv(rbuf,maxbuf,MPI_BYTE,MPI_ANY_SOURCE,MPI_ANY_TAG,world,&status);

    iport = status.MPI_TAG;
    if (iport >= MAXPORT) {
      iport -= MAXPORT;
      doneflag = 1;
    } else doneflag = 0;

    InputPort *ip = &inports[iport];
    if (ip->status != OPEN_PORT)
      phish_error("Received datum on closed or unused port");
    lastport = iport;

    if (doneflag) {
      ip->donecount++;
      if (ip->donecount == ip->donemax) {
	ip->status = CLOSED_PORT;
	if (ip->donefunc) (*ip->donefunc)();
	donecount++;
	if (donecount == ninports) {
	  if (alldonefunc) (*alldonefunc)();
	  return;
	}
      }

    } else {
      rcount++;
      if (ip->datumfunc) {
	MPI_Get_count(&status,MPI_BYTE,&nrbytes);
	nrfields = *(int *) rbuf;
	rptr = rbuf + sizeof(int);
	nunpack = 0;
	(*ip->datumfunc)(nrfields);
      }
    }
  }
}

/* ----------------------------------------------------------------------
   infinite loop on incoming datums
   non-blocking MPI_Iprobe() for a datum
   if no datum, return to caller via probefunc() so app can do work
   if datum, check for DONE message, else callback to datumfunc()
------------------------------------------------------------------------- */

void phish_probe(void (*probefunc)())
{
  int flag,iport,doneflag;
  MPI_Status status;

  if (!checkflag) phish_error("Phish_check has not been called");
  if (!probefunc) phish_error("Phish_probe callback cannot be NULL");

  while (1) {
    MPI_Iprobe(MPI_ANY_SOURCE,MPI_ANY_TAG,world,&flag,&status);
    if (flag) {
      MPI_Recv(rbuf,maxbuf,MPI_BYTE,MPI_ANY_SOURCE,MPI_ANY_TAG,world,&status);

      iport = status.MPI_TAG;
      if (iport >= MAXPORT) {
	iport -= MAXPORT;
	doneflag = 1;
      } else doneflag = 0;

      InputPort *ip = &inports[iport];
      if (ip->status != OPEN_PORT)
	phish_error("Received datum on closed or unused port");
      lastport = iport;

      if (doneflag) {
	ip->donecount++;
	if (ip->donecount == ip->donemax) {
	  ip->status = CLOSED_PORT;
	  if (ip->donefunc) (*ip->donefunc)();
	  donecount++;
	  if (donecount == ninports) {
	    if (alldonefunc) (*alldonefunc)();
	    return;
	  }
	}

      } else {
	rcount++;
	if (ip->datumfunc) {
	  MPI_Get_count(&status,MPI_BYTE,&nrbytes);
	  nrfields = *(int *) rbuf;
	  rptr = rbuf + sizeof(int);
	  nunpack = 0;
	  (*ip->datumfunc)(nrfields);
	}
      }
    } else (*probefunc)();
  }
}

/* ----------------------------------------------------------------------
   check for a message and recv it if there is one
   no datum callback is invoked, even if one is defined
   this allows app to request datums explicitly
     as alternative to phish_loop() and the callback it invokes
   return 0 if no message
   return -1 for done message, after invoking done callbacks
   return N for # fields in datum
------------------------------------------------------------------------- */

int phish_recv()
{
  if (!checkflag) phish_error("Phish_check has not been called");

  int flag;
  MPI_Status status;

  MPI_Iprobe(MPI_ANY_SOURCE,MPI_ANY_TAG,world,&flag,&status);
  if (!flag) return 0;

  MPI_Recv(rbuf,maxbuf,MPI_BYTE,MPI_ANY_SOURCE,MPI_ANY_TAG,world,&status);

  int doneflag;
  int iport = status.MPI_TAG;
  if (iport >= MAXPORT) {
    iport -= MAXPORT;
    doneflag = 1;
  } else doneflag = 0;
    
  InputPort *ip = &inports[iport];
  if (ip->status != OPEN_PORT)
    phish_error("Received datum on closed or unused port");
  lastport = iport;

  if (doneflag) {
    ip->donecount++;
    if (ip->donecount == ip->donemax) {
      ip->status = CLOSED_PORT;
      if (ip->donefunc) (*ip->donefunc)();
      donecount++;
      if (donecount == ninports && alldonefunc) (*alldonefunc)();
    }
    return -1;
  }

  rcount++;
  MPI_Get_count(&status,MPI_BYTE,&nrbytes);
  nrfields = *(int *) rbuf;
  rptr = rbuf + sizeof(int);
  nunpack = 0;
  return nrfields;
}

/* ----------------------------------------------------------------------
   send datum packed in sbuf via output port iport to a downstream proc
------------------------------------------------------------------------- */

void phish_send(int iport)
{
  if (iport < 0 || iport >= MAXPORT) 
    phish_error("Invalid port ID for phish_send");

  OutPort *op = &outports[iport];
  if (op->status == UNUSED_PORT) return;
  if (op->status == CLOSED_PORT) 
    phish_error("Using phish_send with closed port");

  scount++;

  // setup send buffer

  *(int *) sbuf = npack;
  nsbytes = sptr - sbuf;

  // loop over connections
  // send datum to connection receiver via send()

  for (int iconnect = 0; iconnect < op->nconnect; iconnect++)
    send(&op->connects[iconnect]);

  // reset send buffer

  sptr = sbuf + sizeof(int);
  npack = 0;
}

/* ----------------------------------------------------------------------
   send datum packed in sbuf via output port iport to a downstream proc
   choose proc based on hash of key of length keybytes
------------------------------------------------------------------------- */

void phish_send_key(int iport, char *key, int keybytes)
{
  if (iport < 0 || iport >= MAXPORT)
    phish_error("Invalid port ID for phish_send_key");

  OutPort *op = &outports[iport];
  if (op->status == UNUSED_PORT) return;
  if (op->status == CLOSED_PORT) 
    phish_error("Using phish_send_key with closed port");

  scount++;

  // setup send buffer

  *(int *) sbuf = npack;
  nsbytes = sptr - sbuf;

  // loop over connections
  // send datum to connection receiver via send()
  // for HASHED style, use hashing key to compute processor offset & send datum
  // non-HASHED styles just invoke send()

  for (int iconnect = 0; iconnect < op->nconnect; iconnect++) {
    OutConnect *oc = &op->connects[iconnect];

    switch (oc->style) {
    case HASHED:
      {
	int tag = oc->recvport;
	int offset = hashlittle(key,keybytes,oc->nrecv) % oc->nrecv;
	if (safe) 
	  MPI_Ssend(sbuf,nsbytes,MPI_BYTE,oc->recvfirst+offset,tag,world);
	else MPI_Send(sbuf,nsbytes,MPI_BYTE,oc->recvfirst+offset,tag,world);
      }
      break;
      
    default:
      send(oc);
    }
  }

  // reset send buffer

  sptr = sbuf + sizeof(int);
  npack = 0;
}

/* ----------------------------------------------------------------------
   send datum packed in sbuf via output port iport to a downstream proc
   set proc to send to via receiver
------------------------------------------------------------------------- */

void phish_send_direct(int iport, int receiver)
{
  if (iport < 0 || iport >= MAXPORT) 
    phish_error("Invalid port ID for phish_send");

  OutPort *op = &outports[iport];
  if (op->status == UNUSED_PORT) return;
  if (op->status == CLOSED_PORT) 
    phish_error("Using phish_send with closed port");

  scount++;

  // setup send buffer

  *(int *) sbuf = npack;
  nsbytes = sptr - sbuf;

  // loop over connections
  // send datum to connection receiver via send()
  // for DIRECT style, send datum to proc = recvfirst + receiver
  // non-DIRECT styles just invoke send()

  for (int iconnect = 0; iconnect < op->nconnect; iconnect++) {
    OutConnect *oc = &op->connects[iconnect];

    switch (oc->style) {
    case DIRECT:
      {
	int tag = oc->recvport;
	if (receiver < 0 || receiver >= oc->nrecv)
	  phish_error("Invalid receiver for phish_send_direct");
	if (safe)
	  MPI_Ssend(sbuf,nsbytes,MPI_BYTE,oc->recvfirst+receiver,tag,world);
	else MPI_Send(sbuf,nsbytes,MPI_BYTE,oc->recvfirst+receiver,tag,world);
      }
      break;
      
    default:
      send(oc);
    }
  }

  // reset send buffer

  sptr = sbuf + sizeof(int);
  npack = 0;
}

/* ----------------------------------------------------------------------
   send datum packed in sbuf to downstream proc(s)
   internal function, NOT a function in the PHISH API
------------------------------------------------------------------------- */

void send(OutConnect *oc)
{
  int tag = oc->recvport;

  // send datum to appropriate receiving proc depending on connection style

  switch (oc->style) {
  case SINGLE:
  case PAIRED:
  case RING:
    if (safe) MPI_Ssend(sbuf,nsbytes,MPI_BYTE,oc->recvone,tag,world);
    else MPI_Send(sbuf,nsbytes,MPI_BYTE,oc->recvone,tag,world);
    break;

  case ROUNDROBIN:
    if (safe) 
      MPI_Ssend(sbuf,nsbytes,MPI_BYTE,oc->recvfirst+oc->offset,tag,world);
    else MPI_Send(sbuf,nsbytes,MPI_BYTE,oc->recvfirst+oc->offset,tag,world);
    oc->offset++;
    if (oc->offset == oc->nrecv) oc->offset = 0;
    break;

  case CHAIN:
    if (oc->nrecv) {
      if (safe) MPI_Ssend(sbuf,nsbytes,MPI_BYTE,oc->recvone,tag,world);
      else MPI_Send(sbuf,nsbytes,MPI_BYTE,oc->recvone,tag,world);
    }
    break;

  case BCAST:
    for (int i = 0; i < oc->nrecv; i++)
      if (safe) MPI_Ssend(sbuf,nsbytes,MPI_BYTE,oc->recvfirst+i,tag,world);
      else MPI_Send(sbuf,nsbytes,MPI_BYTE,oc->recvfirst+i,tag,world);
    break;

  // error if called for HASHED or DIRECT

  case HASHED:
    phish_error("Cannot use phish_send for HASHED output");
    break;
  case DIRECT:
    phish_error("Cannot use phish_send for DIRECT output");
    break;
  }
}

/* ----------------------------------------------------------------------
   reset the receiver proc that I send to on output port iport
   only valid for connection style RING
   used by application to permute the ordering of the ring
------------------------------------------------------------------------- */

void phish_reset_receiver(int iport, int receiver)
{
  if (iport < 0 || iport >= MAXPORT) 
    phish_error("Invalid port ID for phish_reset_receiver");
  OutPort *op = &outports[iport];
  if (op->status == UNUSED_PORT || op->status == CLOSED_PORT) 
    phish_error("Using phish_reset_receiver with unused or closed port");

  for (int iconnect = 0; iconnect < op->nconnect; iconnect++) {
    OutConnect *oc = &op->connects[iconnect];
    if (oc->style == RING) {
      if (receiver < 0 || receiver >= oc->nrecv)
	phish_error("Invalid receiver proc in phish_reset_receiver");
      oc->recvone = oc->recvfirst + receiver;
    }
  }
}

/* ----------------------------------------------------------------------
   pack the sbuf with values
   first field = value type
   second field (only for arrays) = # of values
   third field = value(s)
   phish_pack_helper(), phish_pack_array_helper() are internal functions,
     NOT functions in the PHISH API
------------------------------------------------------------------------- */

// NOTE: can we use int32_t instead of uint32 everywhere?
//       so that users can use "int" in their apps,
//       unless on a 8-byte int machine

void phish_pack_datum(char *buf, uint32_t len)
{
  if (len > maxbuf) phish_error("Send buffer overflow");

  memcpy(sbuf,buf,len);
  sptr = sbuf + len;
  npack = *(int *) sbuf;
}

void phish_pack_raw(char *buf, uint32_t len)
{
  if (sptr + 2*sizeof(uint32_t) + len - sbuf > maxbuf)
    phish_error("Send buffer overflow");

  *(uint32_t *) sptr = PHISH_RAW;
  sptr += sizeof(uint32_t);
  *(uint32_t *) sptr = len;
  sptr += sizeof(uint32_t);
  memcpy(sptr,buf,len);
  sptr += len;
  npack++;
}

template<typename T>
inline void phish_pack_helper(const T& value, int data_type)
{
  if (sptr + sizeof(uint32_t) + sizeof(T) - sbuf > maxbuf)
    phish_error("Send buffer overflow");

  *(uint32_t *) sptr = data_type;
  sptr += sizeof(uint32_t);
  *(T *) sptr = value;
  sptr += sizeof(T);
  npack++;
}

void phish_pack_char(char value)
{
  phish_pack_helper(value,PHISH_CHAR);
}

void phish_pack_int8(int8_t value)
{
  phish_pack_helper(value,PHISH_INT8);
}

void phish_pack_int16(int16_t value)
{
  phish_pack_helper(value,PHISH_INT16);
}

void phish_pack_int32(int32_t value)
{
  phish_pack_helper(value,PHISH_INT32);
}

void phish_pack_int64(int64_t value)
{
  phish_pack_helper(value,PHISH_INT64);
}

void phish_pack_uint8(uint8_t value)
{
  phish_pack_helper(value,PHISH_UINT8);
}

void phish_pack_uint16(uint16_t value)
{
  phish_pack_helper(value,PHISH_UINT16);
}

void phish_pack_uint32(uint32_t value)
{
  phish_pack_helper(value,PHISH_UINT32);
}

void phish_pack_uint64(uint64_t value)
{
  phish_pack_helper(value,PHISH_UINT64);
}

void phish_pack_float(float value)
{
  phish_pack_helper(value,PHISH_FLOAT);
}

void phish_pack_double(double value)
{
  phish_pack_helper(value,PHISH_DOUBLE);
}

void phish_pack_string(char *str)
{
  int nbytes = strlen(str) + 1;
  if (sptr + 2*sizeof(uint32_t) + nbytes - sbuf > maxbuf)
    phish_error("Send buffer overflow");

  *(uint32_t *) sptr = PHISH_STRING;
  sptr += sizeof(uint32_t);
  *(uint32_t *) sptr = nbytes;
  sptr += sizeof(uint32_t);
  memcpy(sptr,str,nbytes);
  sptr += nbytes;
  npack++;
}

template<typename T>
inline void phish_pack_array_helper(T *vec, uint32_t n, int data_type)
{
  int nbytes = n*sizeof(T);
  if (sptr + 2*sizeof(uint32_t) + nbytes - sbuf > maxbuf)
    phish_error("Send buffer overflow");

  *(uint32_t *) sptr = data_type;
  sptr += sizeof(uint32_t);
  *(uint32_t *) sptr = n;
  sptr += sizeof(uint32_t);
  memcpy(sptr,vec,nbytes);
  sptr += nbytes;
  npack++;
}

void phish_pack_int8_array(int8_t *vec, uint32_t n)
{
  phish_pack_array_helper(vec,n,PHISH_INT8_ARRAY);
}

void phish_pack_int16_array(int16_t *vec, uint32_t n)
{
  phish_pack_array_helper(vec,n,PHISH_INT16_ARRAY);
}

void phish_pack_int32_array(int32_t *vec, uint32_t n)
{
  phish_pack_array_helper(vec,n,PHISH_INT32_ARRAY);
}

void phish_pack_int64_array(int64_t *vec, uint32_t n)
{
  phish_pack_array_helper(vec,n,PHISH_INT64_ARRAY);
}

void phish_pack_uint8_array(uint8_t *vec, uint32_t n)
{
  phish_pack_array_helper(vec,n,PHISH_UINT8_ARRAY);
}

void phish_pack_uint16_array(uint16_t *vec, uint32_t n)
{
  phish_pack_array_helper(vec,n,PHISH_UINT16_ARRAY);
}

void phish_pack_uint32_array(uint32_t *vec, uint32_t n)
{
  phish_pack_array_helper(vec,n,PHISH_UINT32_ARRAY);
}

void phish_pack_uint64_array(uint64_t *vec, uint32_t n)
{
  phish_pack_array_helper(vec,n,PHISH_UINT64_ARRAY);
}

void phish_pack_float_array(float *vec, uint32_t n)
{
  phish_pack_array_helper(vec,n,PHISH_FLOAT_ARRAY);
}

void phish_pack_double_array(double *vec, uint32_t n)
{
  phish_pack_array_helper(vec,n,PHISH_DOUBLE_ARRAY);
}

/* ----------------------------------------------------------------------
   same as phish_pack_raw(), execpt for PHISH_PICKLE flag
------------------------------------------------------------------------- */

void phish_pack_pickle(char *buf, uint32_t len)
{
  if (sptr + 2*sizeof(uint32_t) + len - sbuf > maxbuf)
    phish_error("Send buffer overflow");

  *(uint32_t *) sptr = PHISH_PICKLE;
  sptr += sizeof(uint32_t);
  *(uint32_t *) sptr = len;
  sptr += sizeof(uint32_t);
  memcpy(sptr,buf,len);
  sptr += len;
  npack++;
}

/* ----------------------------------------------------------------------
   process field rbuf, one field at a time
   return field type
   buf = ptr to field
   len = byte count for RAW and STRING (including NULL)
   len = 1 for BYTE, INT, UINT64, DOUBLE
   len = # of array values for ARRAY types
   PHISH_PICKLE is same as PHISH_RAW
------------------------------------------------------------------------- */

int phish_unpack(char **buf, uint32_t *len)
{
  if (nunpack == nrfields) phish_error("Recv buffer empty");

  uint32_t type = *(uint32_t *) rptr;
  rptr += sizeof(uint32_t);

  uint32_t nbytes;
  switch (type) {
  case PHISH_RAW:
    *len = nbytes = *(uint32_t *) rptr;
    rptr += sizeof(uint32_t);
    break;
  case PHISH_CHAR:
    *len = 1;
    nbytes = sizeof(char);
    break;
  case PHISH_INT8:
    *len = 1;
    nbytes = sizeof(int8_t);
    break;
  case PHISH_INT16:
    *len = 1;
    nbytes = sizeof(int16_t);
    break;
  case PHISH_INT32:
    *len = 1;
    nbytes = sizeof(int32_t);
    break;
  case PHISH_INT64:
    *len = 1;
    nbytes = sizeof(int64_t);
    break;
  case PHISH_UINT8:
    *len = 1;
    nbytes = sizeof(uint8_t);
    break;
  case PHISH_UINT16:
    *len = 1;
    nbytes = sizeof(uint16_t);
    break;
  case PHISH_UINT32:
    *len = 1;
    nbytes = sizeof(uint32_t);
    break;
  case PHISH_UINT64:
    *len = 1;
    nbytes = sizeof(uint64_t);
    break;
  case PHISH_FLOAT:
    *len = 1;
    nbytes = sizeof(float);
    break;
  case PHISH_DOUBLE:
    *len = 1;
    nbytes = sizeof(double);
    break;
  case PHISH_STRING:
    *len = nbytes = *(uint32_t *) rptr;
    rptr += sizeof(uint32_t);
    break;
  case PHISH_INT8_ARRAY:
    *len = *(uint32_t *) rptr;
    rptr += sizeof(uint32_t);
    nbytes = *len * sizeof(int8_t);
    break;
  case PHISH_INT16_ARRAY:
    *len = *(uint32_t *) rptr;
    rptr += sizeof(uint32_t);
    nbytes = *len * sizeof(int16_t);
    break;
  case PHISH_INT32_ARRAY:
    *len = *(uint32_t *) rptr;
    rptr += sizeof(uint32_t);
    nbytes = *len * sizeof(int32_t);
    break;
  case PHISH_INT64_ARRAY:
    *len = *(uint32_t *) rptr;
    rptr += sizeof(uint32_t);
    nbytes = *len * sizeof(int64_t);
    break;
  case PHISH_UINT8_ARRAY:
    *len = *(uint32_t *) rptr;
    rptr += sizeof(uint32_t);
    nbytes = *len * sizeof(uint8_t);
    break;
  case PHISH_UINT16_ARRAY:
    *len = *(uint32_t *) rptr;
    rptr += sizeof(uint32_t);
    nbytes = *len * sizeof(uint16_t);
    break;
  case PHISH_UINT32_ARRAY:
    *len = *(uint32_t *) rptr;
    rptr += sizeof(uint32_t);
    nbytes = *len * sizeof(uint32_t);
    break;
  case PHISH_UINT64_ARRAY:
    *len = *(uint32_t *) rptr;
    rptr += sizeof(uint32_t);
    nbytes = *len * sizeof(uint64_t);
    break;
  case PHISH_FLOAT_ARRAY:
    *len = *(uint32_t *) rptr;
    rptr += sizeof(uint32_t);
    nbytes = *len * sizeof(float);
    break;
  case PHISH_DOUBLE_ARRAY:
    *len = *(uint32_t *) rptr;
    rptr += sizeof(uint32_t);
    nbytes = *len * sizeof(double);
    break;
  case PHISH_PICKLE:
    *len = nbytes = *(uint32_t *) rptr;
    rptr += sizeof(uint32_t);
    break;
  }

  *buf = rptr;
  rptr += nbytes;

  return type;
}

/* ----------------------------------------------------------------------
   return info about entire received datum
   return input port the datum was received on
   buf = ptr to datum
   len = total size of received datum
------------------------------------------------------------------------- */

int phish_datum(char **buf, uint32_t *len)
{
  *buf = rbuf;
  *len = nrbytes;
  return lastport;
}

/* ----------------------------------------------------------------------
   add current datum to end of internal queue
   return count of queued datums
------------------------------------------------------------------------- */

int phish_queue()
{
  if (nqueue == maxqueue) {
    maxqueue += DELTAQUEUE;
    queue = (Queue *) realloc(queue,maxqueue*sizeof(Queue));
    if (!queue) phish_error("Phish_queue ran out of memory");
  }

  queue[nqueue].datum = new char[nrbytes];
  if (!queue[nqueue].datum) phish_error("Phish_queue ran out of memory");
  memcpy(queue[nqueue].datum,rbuf,nrbytes);
  queue[nqueue].nbytes = nrbytes;
  queue[nqueue].iport = lastport;
  nqueue++;

  return nqueue;
}

/* ----------------------------------------------------------------------
   return count of internally queued datums
------------------------------------------------------------------------- */

int phish_nqueue()
{
  return nqueue;
}

/* ----------------------------------------------------------------------
   remove datum N from internal queue
   copy it to receive buffer, as if just received
   compress queue
   return # of values in the datum
------------------------------------------------------------------------- */

int phish_dequeue(int n)
{
  if (n < 0 || n >= nqueue) phish_error("Invalid phish_dequeue");

 nrbytes = queue[n].nbytes;
 memcpy(rbuf,queue[n].datum,nrbytes);
 delete [] queue[n].datum;
 nrfields = *(int *) rbuf;
 rptr = rbuf + sizeof(int);
 nunpack = 0;
 lastport = queue[n].iport;

 memmove(&queue[n],&queue[n+1],(nqueue-n-1)*sizeof(Queue));
 nqueue--;

 return nrfields;
}

/* ----------------------------------------------------------------------
   return info about minnow counts and input/output ports
------------------------------------------------------------------------- */

int phish_query(const char *keyword, int flag1, int flag2)
{
  if (!initflag) phish_error("Phish_init has not been called");

  if (strcmp(keyword,"idlocal") == 0) return idlocal;
  else if (strcmp(keyword,"nlocal") == 0) return nlocal;
  else if (strcmp(keyword,"idglobal") == 0) return idglobal;
  else if (strcmp(keyword,"nglobal") == 0) return nglobal;
  else if (strcmp(keyword,"inport/status") == 0) {
    int iport = flag1;
    if (iport < 0 || iport >= MAXPORT) phish_error("Invalid phish_query");
    return inports[iport].status;
  } else if (strcmp(keyword,"inport/connections") == 0) {
    int iport = flag1;
    if (iport < 0 || iport >= MAXPORT) phish_error("Invalid phish_query");
    if (inports[iport].status == UNUSED_PORT) return -1;
    return inports[iport].nconnect;
  } else if (strcmp(keyword,"inport/nminnows") == 0) {
    int iport = flag1;
    if (iport < 0 || iport >= MAXPORT) phish_error("Invalid phish_query");
    if (inports[iport].status == UNUSED_PORT) return -1;
    int iconnect = flag2;
    if (iconnect < 0 || iconnect >= inports[iport].nconnect) return -1;
    return inports[iport].connects[iconnect].nsend;
  } else if (strcmp(keyword,"outport/status") == 0) {
    int iport = flag1;
    if (iport < 0 || iport >= MAXPORT) phish_error("Invalid phish_query");
    return outports[iport].status;
  } else if (strcmp(keyword,"outport/connections") == 0) {
    int iport = flag1;
    if (iport < 0 || iport >= MAXPORT) phish_error("Invalid phish_query");
    if (outports[iport].status == UNUSED_PORT) return -1;
    return outports[iport].nconnect;
  } else if (strcmp(keyword,"outport/nminnows") == 0) {
    int iport = flag1;
    if (iport < 0 || iport >= MAXPORT) phish_error("Invalid phish_query");
    if (outports[iport].status == UNUSED_PORT) return -1;
    int iconnect = flag2;
    if (iconnect < 0 || iconnect >= outports[iport].nconnect) return -1;
    return outports[iport].connects[iconnect].nrecv;
  } else if (strcmp(keyword,"outport/direct") == 0) {
    int iport = flag1;
    if (iport < 0 || iport >= MAXPORT) phish_error("Invalid phish_query");
    if (outports[iport].status == UNUSED_PORT) return -1;
    for (int iconnect = 0; iconnect < outports[iport].nconnect; iconnect++)
      if (outports[iport].connects[iconnect].style == DIRECT) 
	return outports[iport].connects[iconnect].nrecv;
    return -1;
  } else phish_error("Invalid phish_query");

  return 0;
}

/* ---------------------------------------------------------------------- */

void phish_error(const char *str)
{
  fprintf(stderr,"PHISH MPI ERROR: Minnow %s ID %s # %d: %s\n",
	  exename,idminnow,idglobal,str);

  // NOTE: what is the flag?
  // NOTE: should we set initflag to 0, so callback function
  //       cannot call PHISH library?

  int flag = 0;
  if (abortfunc) (*abortfunc)(flag);
  MPI_Abort(world,1);
}

/* ---------------------------------------------------------------------- */

void phish_warn(const char *str)
{
  fprintf(stderr,"PHISH MPI WARNING: Minnow %s ID %s # %d: %s\n",
	  exename,idminnow,idglobal,str);
}

/* ---------------------------------------------------------------------- */

void phish_abort()
{
  MPI_Abort(world,1);
}

/* ---------------------------------------------------------------------- */

double phish_timer()
{
  return MPI_Wtime();
}

/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */

/* ----------------------------------------------------------------------
   internal functions, NOT functions in the PHISH API
------------------------------------------------------------------------- */

/* ----------------------------------------------------------------------
   print stats about datums received/sent by minnow
------------------------------------------------------------------------- */

void stats()
{
  // NOTE: why print this to stderr instead of stdout?
  fprintf(stderr,"Stats: Minnow %s ID %s # %d: %lu %lu datums recv/sent\n",
	  exename,idminnow,idglobal,rcount,scount);
}

/* ----------------------------------------------------------------------
   allocate/deallocate memory for MAXPORT input and output ports
------------------------------------------------------------------------- */

void allocate_ports()
{
  inports = new InputPort[MAXPORT];
  for (int i = 0; i < MAXPORT; i++) {
    inports[i].status = UNUSED_PORT;
    inports[i].donecount = 0;
    inports[i].donemax = 0;
    inports[i].nconnect = 0;
    inports[i].connects = NULL;
  }

  outports = new OutPort[MAXPORT];
  for (int i = 0; i < MAXPORT; i++) {
    outports[i].status = UNUSED_PORT;
    outports[i].nconnect = 0;
    outports[i].connects = NULL;
  }
}

/* ---------------------------------------------------------------------- */

void deallocate_ports()
{
  for (int i = 0; i < MAXPORT; i++)
    if (inports[i].status != UNUSED_PORT) {
      for (int j = 0; j < inports[i].nconnect; j++)
	delete [] inports[i].connects[j].host;
      free(inports[i].connects);
    }
  delete [] inports;
  for (int i = 0; i < MAXPORT; i++)
    if (outports[i].status != UNUSED_PORT) free(outports[i].connects);
  delete [] outports;
}
