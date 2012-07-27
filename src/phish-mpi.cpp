/* ----------------------------------------------------------------------
   PHISH = Parallel Harness for Informatic Stream Hashing
   http://www.sandia.gov/~sjplimp/phish.html
   Steve Plimpton, sjplimp@sandia.gov, Sandia National Laboratories

   Copyright (2012) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under 
   the modified Berkeley Software Distribution (BSD) License.

   See the README file in the top-level PHISH directory.
------------------------------------------------------------------------- */

// MPI version of PHISH library

#include "mpi.h"
#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "stdint.h"
#include "phish.h"
#include "phish-common.h"
#include "hash.h"

#include <sstream>

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

#define SELFOVERHEAD 100 // per-message MPI overhead in selfbuf

typedef void (DatumFunc)(int);         // callback prototypes
typedef void (DoneFunc)();
typedef void (AbortFunc)(int*);

/* ---------------------------------------------------------------------- */
// variables local to single PHISH instance

MPI_Comm world;           // MPI communicator
int me,nprocs;            // MPI rank and total # of procs

int maxbuf;               // max allowed datum size in bytes
int memchunk;             // max allowed datum size in Kbytes
int safe;                 // do safe MPI sends every this many sends (0 = never)

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

int self;                  // 1 if possibly send message to self, else 0
int selfnum;               // # of queued self messsages that can be stored
char *selfbuf;             // buffer for messages to self, used by MPI

// queued datums

struct Queue {             // one queued datum
  char *datum;             // copy of entire datum
  int nbytes;              // byte size of datum
  int iport;               // port the datum was received on
};

Queue *queue;              // internal queue
int nqueue;                // # of datums in queue
int maxqueue;              // max # of datums queue can hold

// internal function prototypes

void send(OutConnect *);
void allocate_ports();
void deallocate_ports();

/* ---------------------------------------------------------------------- */

int phish_init(int *argc, char ***argv)
{
  phish_assert_not_initialized();

  // initialize MPI

  MPI_Init(argc,argv);

  world = MPI_COMM_WORLD;
  MPI_Comm_rank(world,&me);
  MPI_Comm_size(world,&nprocs);

  // library status

  g_initialized = true;
  lastport = -1;
  nqueue = maxqueue = 0;
  queue = NULL;

  // default settings

  memchunk = 1;
  safe = 0;
  selfnum = 10;

  // port allocation

  allocate_ports();

  // parse input args and setup communication port data structs

  std::vector<std::string> arguments(*argv, *argv + *argc);
  std::vector<std::string> kept_arguments;

  g_executable = arguments[0];

  while (arguments.size()) {
    const std::string argument = pop_argument(arguments);
    if (argument == "--phish-backend") {
      const std::string backend = pop_argument(arguments);
      if (backend != "mpi") {
        std::ostringstream message;
        message << "Incompatible backend: expected mpi, using " 
                << backend << ".";
        phish_return_error(message.str().c_str(),-1);
      }

    } else if (argument == "--phish-minnow") {
      if (arguments.size() < 3)
        phish_return_error("Invalid command-line args in phish_init", -1);

      g_school_id = pop_argument(arguments);
      g_local_count = atoi(pop_argument(arguments).c_str());
      int nprev = atoi(pop_argument(arguments).c_str());
      g_local_id = me - nprev;
      g_global_id = me;
      g_global_count = nprocs;

    } else if (argument == "--phish-memory") {
      if (arguments.size() < 1)
        phish_return_error("Invalid command-line args in phish_init", -1);
      memchunk = atoi(pop_argument(arguments).c_str());
      if (memchunk < 0) 
        phish_return_error("Invalid command-line args in phish_init", -1);

    } else if(argument == "--phish-safe") {
      if (arguments.size() < 1)
        phish_return_error("Invalid command-line args in phish_init", -1);
      safe = atoi(pop_argument(arguments).c_str());
      if (safe < 0)
        phish_return_error("Invalid command-line args in phish_init", -1);

    } else if(argument == "--phish-self") {
      if (arguments.size() < 1)
        phish_return_error("Invalid command-line args in phish_init", -1);
      selfnum = atoi(pop_argument(arguments).c_str());
      if (selfnum < 0)
        phish_return_error("Invalid command-line args in phish_init", -1);

    } else if (argument == "--phish-in") {
      if (arguments.size() < 7)
        phish_return_error("Invalid command-line args in phish_init", -1);
      int style,sprocs,sfirst,sport,rprocs,rfirst,rport;
      char *host = NULL;

      sprocs = atoi(pop_argument(arguments).c_str());
      sfirst = atoi(pop_argument(arguments).c_str());
      sport = atoi(pop_argument(arguments).c_str());
      const std::string raw_style = pop_argument(arguments);
      rprocs = atoi(pop_argument(arguments).c_str());
      rfirst = atoi(pop_argument(arguments).c_str());
      rport = atoi(pop_argument(arguments).c_str());

      if (raw_style == "single") style = SINGLE;
      else if (raw_style == "paired") style = PAIRED;
      else if (raw_style == "hashed") style = HASHED;
      else if (raw_style == "roundrobin") style = ROUNDROBIN;
      else if (raw_style == "direct") style = DIRECT;
      else if (raw_style == "bcast") style = BCAST;
      else if (raw_style == "chain") style = CHAIN;
      else if (raw_style == "ring") style = RING;
#ifdef PHISH_MPI_ZMQ
      else if (raw_style == "subscribe") {
        style = SUBSCRIBE;
	int n = strlen(args[iarg+4]) - strlen("subscribe/") + 1;
	host = new char[n];
	strcpy(host,&argv[iarg+4][strlen("subscribe/")]);
      }
#endif
      else phish_return_error("Unrecognized in style in phish_init", -1);

      if (rport > MAXPORT)
	phish_return_error("Invalid input port ID in phish_init", -1);
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

    } else if (argument == "--phish-out") {
      if (arguments.size() < 7)
        phish_return_error("Invalid command-line args in phish_init", -1);
      int style,sprocs,sfirst,sport,rprocs,rfirst,rport;
      int tcpport = 0;

      sprocs = atoi(pop_argument(arguments).c_str());
      sfirst = atoi(pop_argument(arguments).c_str());
      sport = atoi(pop_argument(arguments).c_str());
      const std::string raw_style = pop_argument(arguments);
      rprocs = atoi(pop_argument(arguments).c_str());
      rfirst = atoi(pop_argument(arguments).c_str());
      rport = atoi(pop_argument(arguments).c_str());

      if (raw_style == "single") style = SINGLE;
      else if (raw_style == "paired") style = PAIRED;
      else if (raw_style == "hashed") style = HASHED;
      else if (raw_style == "roundrobin") style = ROUNDROBIN;
      else if (raw_style == "direct") style = DIRECT;
      else if (raw_style == "bcast") style = BCAST;
      else if (raw_style == "chain") style = CHAIN;
      else if (raw_style == "ring") style = RING;
#ifdef PHISH_MPI_ZMQ
      else if (strstr(argv[iarg+4],"publish") == argv[iarg+4]) {
	style = PUBLISH;
	tcpport = atoi(&argv[iarg+4][strlen("publish/")]);
      }
#endif
      else phish_return_error("Unrecognized out style in phish_init", -1);

      if (sport > MAXPORT)
	phish_return_error("Invalid output port ID in phish_init", -1);
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
	// set nrecv and recvfirst so can invoke set("ring/receiver")
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
    }

    else kept_arguments.push_back(argument);
  }

  // check if possibly send message to self

  self = 0;
  for (int iport = 0; iport < MAXPORT; iport++) {
    if (outports[iport].status == UNUSED_PORT) continue;
    for (int iconnect = 0; iconnect < outports[iport].nconnect; iconnect++) {
      OutConnect *oc = &outports[iport].connects[iconnect];
      switch (oc->style) {
      case SINGLE:
      case PAIRED:
        if (me == oc->recvone) self = 1;
        break;
      case HASHED:
      case ROUNDROBIN:
      case DIRECT:
      case BCAST:
        if (me >= oc->recvfirst && me < oc->recvfirst + oc->nrecv) self = 1;
        break;
      default:
        break;
      }
    }
  }

  // cleanup argc & argv

  *argc = get_argc(kept_arguments);
  *argv = get_argv(kept_arguments);

  // get this minnow's host

  char host_buffer[MPI_MAX_PROCESSOR_NAME];
  int length = 0;
  MPI_Get_processor_name(host_buffer,&length);
  g_host = std::string(host_buffer,length);

  // memory allocation/initialization for datum buffers
  // if self, attach buffer to MPI for sending to self

  maxbuf = memchunk * KBYTE;
  sbuf = (char *) malloc(maxbuf*sizeof(char));
  rbuf = (char *) malloc(maxbuf*sizeof(char));
  if (!sbuf || !rbuf) phish_return_error("Malloc of datum buffers failed", -1);

  if (self) {
    int n = selfnum * (maxbuf*KBYTE + SELFOVERHEAD);
    selfbuf = (char *) malloc(n*sizeof(char));
    MPI_Buffer_attach(selfbuf,n);
  } else selfbuf = NULL;

  // set send buffer ptr for initial datum

  sptr = sbuf + sizeof(int32_t);
  npack = 0;

  return 0;
}

/* ---------------------------------------------------------------------- */

int phish_exit()
{
  phish_assert_initialized();
  phish_assert_checked();

  // display stats

  phish_stats();

  // warn if any input port is still open

  for (int i = 0; i < MAXPORT; i++)
    if (inports[i].status == OPEN_PORT)
      phish_warn("Exiting with input port still open");

  // close all output ports

  for (int i = 0; i < MAXPORT; i++) phish_close(i);

  // free PHISH memory, including self buffer attached to MPI

  free(sbuf);
  free(rbuf);

  if (self) {
    char *tmpbuf;
    int tmpsize;
    MPI_Buffer_detach(&tmpbuf,&tmpsize);
    free(selfbuf);
  }

  for (int i = 0; i < nqueue; i++) delete [] queue[i].datum;
  free(queue);

  deallocate_ports();

  // shut-down MPI

  MPI_Finalize();

  return 0;
}

/* ----------------------------------------------------------------------
   setup single input port iport
   reqflag = 1 if port must be used by input script
------------------------------------------------------------------------- */

int phish_input(int iport, void (*datumfunc)(int), void (*donefunc)(), 
                int reqflag)
{
  phish_assert_initialized();
  phish_assert_not_checked();

  if (iport < 0 || iport > MAXPORT)
    phish_return_error("Invalid port ID in phish_input",-1);

  if (reqflag && inports[iport].status == UNUSED_PORT)
    phish_return_error("Input script does not use a required input port",-1);

  if (inports[iport].status == UNUSED_PORT) return 0;
  inports[iport].status = OPEN_PORT;
  inports[iport].datumfunc = datumfunc;
  inports[iport].donefunc = donefunc;

  return 0;
}

/* ----------------------------------------------------------------------
   setup single output port iport
   no reqflag setting, since script does not have to use the port
------------------------------------------------------------------------- */

int phish_output(int iport)
{
  phish_assert_initialized();
  phish_assert_not_checked();

  if (iport < 0 || iport > MAXPORT)
    phish_return_error("Invalid port ID in phish_output", -1);

  if (outports[iport].status == UNUSED_PORT) return 0;
  outports[iport].status = OPEN_PORT;

  return 0;
}

/* ----------------------------------------------------------------------
   check consistency of input args with ports setup by phish input/output
------------------------------------------------------------------------- */

int phish_check()
{
  phish_assert_initialized();
  phish_assert_not_checked();
  g_checked = true;

  // args processed by phish_init() requested various input ports
  // flagged them as CLOSED, others as UNUSED
  // phish_input() reset CLOSED ports to OPEN
  // error if a port is CLOSED, since phish_input was not called
  // set ninports = # of used input ports
  // initialize donecount before datum exchanges begin

  ninports = 0;
  for (int i = 0; i < MAXPORT; i++) {
    if (inports[i].status == CLOSED_PORT)
      phish_return_error("Input script uses an undefined input port",-1);
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
      phish_return_error("Input script uses an undefined output port",-1);
    if (outports[i].status == OPEN_PORT) noutports++;
  }

  // stats

  return 0;
}

/* ----------------------------------------------------------------------
   close output port iport
------------------------------------------------------------------------- */

int phish_close(int iport)
{
  phish_assert_checked();

  if (iport < 0 || iport >= MAXPORT)
    phish_return_error("Invalid port ID in phish_close", -1);
  OutPort *op = &outports[iport];
  if (op->status != OPEN_PORT) return 0;

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
      if (self && me == oc->recvone)
	MPI_Bsend(NULL,0,MPI_BYTE,oc->recvone,tag,world);
      else if (safe && g_sent_count % safe == 0) 
	MPI_Ssend(NULL,0,MPI_BYTE,oc->recvone,tag,world);
      else MPI_Send(NULL,0,MPI_BYTE,oc->recvone,tag,world);
      break;

    case HASHED:
    case ROUNDROBIN:
    case DIRECT:
    case BCAST:
      for (int i = 0; i < oc->nrecv; i++)
        if (self && me == oc->recvfirst+i)
          MPI_Bsend(NULL,0,MPI_BYTE,oc->recvfirst+i,tag,world);
        else if (safe && g_sent_count % safe == 0)
	  MPI_Ssend(NULL,0,MPI_BYTE,oc->recvfirst+i,tag,world);
        else MPI_Send(NULL,0,MPI_BYTE,oc->recvfirst+i,tag,world);
      break;

    case CHAIN:
      if (oc->nrecv) {
	if (self && me == oc->recvone) 
	  MPI_Bsend(NULL,0,MPI_BYTE,oc->recvone,tag,world);
	else if (safe && g_sent_count % safe == 0) 
	  MPI_Ssend(NULL,0,MPI_BYTE,oc->recvone,tag,world);
	else MPI_Send(NULL,0,MPI_BYTE,oc->recvone,tag,world);
      }
      break;
    }
  }

  outports[iport].status = CLOSED_PORT;

  return 0;
}

/* ----------------------------------------------------------------------
   infinite loop on incoming datums
   blocking MPI_Recv() for a datum
   check datum for DONE message, else callback to datumfunc()
------------------------------------------------------------------------- */

int phish_loop()
{
  phish_assert_checked();

  int iport,doneflag;
  MPI_Status status;

  while (1) {
    MPI_Recv(rbuf,maxbuf,MPI_BYTE,MPI_ANY_SOURCE,MPI_ANY_TAG,world,&status);

    iport = status.MPI_TAG;
    if (iport >= MAXPORT) {
      iport -= MAXPORT;
      doneflag = 1;
    } else doneflag = 0;

    InputPort *ip = &inports[iport];
    if (ip->status != OPEN_PORT)
      phish_return_error("Received datum on closed or unused port", -1);
    lastport = iport;

    if (doneflag) {
      ip->donecount++;
      if (ip->donecount == ip->donemax) {
	ip->status = CLOSED_PORT;
	if (ip->donefunc) (*ip->donefunc)();
	donecount++;
	if (donecount == ninports) {
	  if (g_all_input_ports_closed) (*g_all_input_ports_closed)();
	  return 0;
	}
      }

    } else {
      g_received_count++;
      if (ip->datumfunc) {
	MPI_Get_count(&status,MPI_BYTE,&nrbytes);
	nrfields = (int) (*(int32_t *) rbuf);
	rptr = rbuf + sizeof(int32_t);
	nunpack = 0;
	(*ip->datumfunc)(nrfields);
      }
    }
  }

  return 0;
}

/* ----------------------------------------------------------------------
   infinite loop on incoming datums
   non-blocking MPI_Iprobe() for a datum
   if no datum, return to caller via probefunc() so app can do work
   if datum, check for DONE message, else callback to datumfunc()
------------------------------------------------------------------------- */

int phish_probe(void (*probefunc)())
{
  phish_assert_checked();
  if (!probefunc)
    phish_return_error("phish_probe callback cannot be NULL", -1);

  int flag,iport,doneflag;
  MPI_Status status;

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
	phish_return_error("Received datum on closed or unused port", -1);
      lastport = iport;

      if (doneflag) {
	ip->donecount++;
	if (ip->donecount == ip->donemax) {
	  ip->status = CLOSED_PORT;
	  if (ip->donefunc) (*ip->donefunc)();
	  donecount++;
	  if (donecount == ninports) {
	    if (g_all_input_ports_closed) (*g_all_input_ports_closed)();
	    return 0;
	  }
	}

      } else {
	g_received_count++;
	if (ip->datumfunc) {
	  MPI_Get_count(&status,MPI_BYTE,&nrbytes);
	  nrfields = (int) (*(int *) rbuf);
	  rptr = rbuf + sizeof(int32_t);
	  nunpack = 0;
	  (*ip->datumfunc)(nrfields);
	}
      }
    } else (*probefunc)();
  }

  return 0;
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
  phish_assert_checked();

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
    phish_return_error("Received datum on closed or unused port", -1);
  lastport = iport;

  if (doneflag) {
    ip->donecount++;
    if (ip->donecount == ip->donemax) {
      ip->status = CLOSED_PORT;
      if (ip->donefunc) (*ip->donefunc)();
      donecount++;
      if (donecount == ninports && g_all_input_ports_closed) 
        (*g_all_input_ports_closed)();
    }
    return -1;
  }

  g_received_count++;
  MPI_Get_count(&status,MPI_BYTE,&nrbytes);
  nrfields = (int) (*(int *) rbuf);
  rptr = rbuf + sizeof(int32_t);
  nunpack = 0;
  return nrfields;
}

/* ----------------------------------------------------------------------
   send datum packed in sbuf via output port iport to a downstream proc
------------------------------------------------------------------------- */

void phish_send(int iport)
{
  if (iport < 0 || iport >= MAXPORT) 
    phish_error("Invalid port ID in phish_send");

  OutPort *op = &outports[iport];
  if (op->status == UNUSED_PORT) {
    sptr = sbuf + sizeof(int32_t);
    npack = 0;
    return;
  }
  if (op->status == CLOSED_PORT) 
    phish_error("Using phish_send with closed port");

  g_sent_count++;

  // setup send buffer

  *(int32_t *) sbuf = npack;
  nsbytes = sptr - sbuf;

  // loop over connections
  // send datum to connection receiver via send()

  for (int iconnect = 0; iconnect < op->nconnect; iconnect++)
    send(&op->connects[iconnect]);

  // reset send buffer

  sptr = sbuf + sizeof(int32_t);
  npack = 0;
}

/* ----------------------------------------------------------------------
   send datum packed in sbuf via output port iport to a downstream proc
   choose proc based on hash of key of length keybytes
------------------------------------------------------------------------- */

void phish_send_key(int iport, char *key, int keybytes)
{
  if (iport < 0 || iport >= MAXPORT)
    phish_error("Invalid port ID in phish_send_key");

  OutPort *op = &outports[iport];
  if (op->status == UNUSED_PORT) {
    sptr = sbuf + sizeof(int32_t);
    npack = 0;
    return;
  }
  if (op->status == CLOSED_PORT) 
    phish_error("Using phish_send_key with closed port");

  g_sent_count++;

  // setup send buffer

  *(int32_t *) sbuf = npack;
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
        if (self && me == oc->recvfirst+offset)
          MPI_Bsend(sbuf,nsbytes,MPI_BYTE,oc->recvfirst+offset,tag,world);
	else if (safe && g_sent_count % safe == 0) 
	  MPI_Ssend(sbuf,nsbytes,MPI_BYTE,oc->recvfirst+offset,tag,world);
	else MPI_Send(sbuf,nsbytes,MPI_BYTE,oc->recvfirst+offset,tag,world);
      }
      break;
      
    default:
      send(oc);
    }
  }

  // reset send buffer

  sptr = sbuf + sizeof(int32_t);
  npack = 0;
}

/* ----------------------------------------------------------------------
   send datum packed in sbuf via output port iport to a downstream proc
   set proc to send to via receiver
------------------------------------------------------------------------- */

void phish_send_direct(int iport, int receiver)
{
  if (iport < 0 || iport >= MAXPORT) 
    phish_error("Invalid port ID in phish_send");

  OutPort *op = &outports[iport];
  if (op->status == UNUSED_PORT) {
    sptr = sbuf + sizeof(int32_t);
    npack = 0;
    return;
  }
  if (op->status == CLOSED_PORT) 
    phish_error("Using phish_send with closed port");

  g_sent_count++;

  // setup send buffer

  *(int32_t *) sbuf = npack;
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
	if (self && me == oc->recvfirst+receiver)
	  MPI_Bsend(sbuf,nsbytes,MPI_BYTE,oc->recvfirst+receiver,tag,world);
	else if (safe && g_sent_count % safe == 0)
	  MPI_Ssend(sbuf,nsbytes,MPI_BYTE,oc->recvfirst+receiver,tag,world);
	else MPI_Send(sbuf,nsbytes,MPI_BYTE,oc->recvfirst+receiver,tag,world);
      }
      break;
      
    default:
      send(oc);
    }
  }

  // reset send buffer

  sptr = sbuf + sizeof(int32_t);
  npack = 0;
}

/* ----------------------------------------------------------------------
   send datum packed in sbuf to downstream proc(s)
   internal function, NOT a function in the PHISH API
------------------------------------------------------------------------ */

void send(OutConnect *oc)
{
  int tag = oc->recvport;

  // send datum to appropriate receiving proc depending on connection style

  switch (oc->style) {
  case SINGLE:
  case PAIRED:
  case RING:
    if (self && me == oc->recvone) 
      MPI_Bsend(sbuf,nsbytes,MPI_BYTE,oc->recvone,tag,world);
    else if (safe && g_sent_count % safe == 0) 
      MPI_Ssend(sbuf,nsbytes,MPI_BYTE,oc->recvone,tag,world);
    else MPI_Send(sbuf,nsbytes,MPI_BYTE,oc->recvone,tag,world);
    break;

  case ROUNDROBIN:
    if (self && me == oc->recvfirst+oc->offset) 
      MPI_Bsend(sbuf,nsbytes,MPI_BYTE,oc->recvfirst+oc->offset,tag,world);
    else if (safe && g_sent_count % safe == 0) 
      MPI_Ssend(sbuf,nsbytes,MPI_BYTE,oc->recvfirst+oc->offset,tag,world);
    else MPI_Send(sbuf,nsbytes,MPI_BYTE,oc->recvfirst+oc->offset,tag,world);
    oc->offset++;
    if (oc->offset == oc->nrecv) oc->offset = 0;
    break;

  case CHAIN:
    if (oc->nrecv) {
      if (self && me == oc->recvone)
	MPI_Bsend(sbuf,nsbytes,MPI_BYTE,oc->recvone,tag,world);
      else if (safe && g_sent_count % safe == 0)
	MPI_Ssend(sbuf,nsbytes,MPI_BYTE,oc->recvone,tag,world);
      else MPI_Send(sbuf,nsbytes,MPI_BYTE,oc->recvone,tag,world);
    }
    break;

  case BCAST:
    for (int i = 0; i < oc->nrecv; i++)
      if (self && me == oc->recvfirst+i)
	MPI_Bsend(sbuf,nsbytes,MPI_BYTE,oc->recvfirst+i,tag,world);
      else if (safe && g_sent_count % safe == 0)
	MPI_Ssend(sbuf,nsbytes,MPI_BYTE,oc->recvfirst+i,tag,world);
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
   copy fields of receive buffer into send buffer
------------------------------------------------------------------------- */

void phish_repack()
{
  int nbytes = nrbytes - sizeof(int32_t);
  if (sptr + nbytes - sbuf > maxbuf) phish_error("Send buffer overflow");

  memcpy(sptr,rbuf+sizeof(int32_t),nbytes);
  sptr += nbytes;
  npack += nrfields;
}

/* ----------------------------------------------------------------------
   templated helper functions for packing data into send buffer
   NOT functions in the PHISH API
------------------------------------------------------------------------- */

template<typename T>
inline void phish_pack_helper(const T& value, int data_type)
{
  if (sptr + sizeof(int32_t) + sizeof(T) - sbuf > maxbuf)
    phish_error("Send buffer overflow");

  *(int32_t *) sptr = data_type;
  sptr += sizeof(int32_t);
  *(T *) sptr = value;
  sptr += sizeof(T);
  npack++;
}

template<typename T>
inline void phish_pack_array_helper(T *vec, int32_t n, int data_type)
{
  int nbytes = n*sizeof(T);
  if (sptr + 2*sizeof(int32_t) + nbytes - sbuf > maxbuf)
    phish_error("Send buffer overflow");

  *(int32_t *) sptr = data_type;
  sptr += sizeof(int32_t);
  *(int32_t *) sptr = n;
  sptr += sizeof(int32_t);
  memcpy(sptr,vec,nbytes);
  sptr += nbytes;
  npack++;
}

/* ----------------------------------------------------------------------
   pack the sbuf with values
   first field = value type
   second field = # of values (only for byte-strings and arrays)
   third field = value(s)
   pack_pickle() is same as pack_raw()
------------------------------------------------------------------------- */

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

void phish_pack_raw(char *buf, int32_t len)
{
  if (sptr + 2*sizeof(int32_t) + len - sbuf > maxbuf)
    phish_error("Send buffer overflow");

  *(int32_t *) sptr = PHISH_RAW;
  sptr += sizeof(int32_t);
  *(int32_t *) sptr = len;
  sptr += sizeof(int32_t);
  memcpy(sptr,buf,len);
  sptr += len;
  npack++;
}

void phish_pack_string(char *str)
{
  int nbytes = strlen(str) + 1;
  if (sptr + 2*sizeof(int32_t) + nbytes - sbuf > maxbuf)
    phish_error("Send buffer overflow");

  *(int32_t *) sptr = PHISH_STRING;
  sptr += sizeof(int32_t);
  *(int32_t *) sptr = nbytes;
  sptr += sizeof(int32_t);
  memcpy(sptr,str,nbytes);
  sptr += nbytes;
  npack++;
}

void phish_pack_int8_array(int8_t *vec, int32_t n)
{
  phish_pack_array_helper(vec,n,PHISH_INT8_ARRAY);
}

void phish_pack_int16_array(int16_t *vec, int32_t n)
{
  phish_pack_array_helper(vec,n,PHISH_INT16_ARRAY);
}

void phish_pack_int32_array(int32_t *vec, int32_t n)
{
  phish_pack_array_helper(vec,n,PHISH_INT32_ARRAY);
}

void phish_pack_int64_array(int64_t *vec, int32_t n)
{
  phish_pack_array_helper(vec,n,PHISH_INT64_ARRAY);
}

void phish_pack_uint8_array(uint8_t *vec, int32_t n)
{
  phish_pack_array_helper(vec,n,PHISH_UINT8_ARRAY);
}

void phish_pack_uint16_array(uint16_t *vec, int32_t n)
{
  phish_pack_array_helper(vec,n,PHISH_UINT16_ARRAY);
}

void phish_pack_uint32_array(uint32_t *vec, int32_t n)
{
  phish_pack_array_helper(vec,n,PHISH_UINT32_ARRAY);
}

void phish_pack_uint64_array(uint64_t *vec, int32_t n)
{
  phish_pack_array_helper(vec,n,PHISH_UINT64_ARRAY);
}

void phish_pack_float_array(float *vec, int32_t n)
{
  phish_pack_array_helper(vec,n,PHISH_FLOAT_ARRAY);
}

void phish_pack_double_array(double *vec, int32_t n)
{
  phish_pack_array_helper(vec,n,PHISH_DOUBLE_ARRAY);
}

void phish_pack_pickle(char *buf, int32_t len)
{
  if (sptr + 2*sizeof(int32_t) + len - sbuf > maxbuf)
    phish_error("Send buffer overflow");

  *(int32_t *) sptr = PHISH_PICKLE;
  sptr += sizeof(int32_t);
  *(int32_t *) sptr = len;
  sptr += sizeof(int32_t);
  memcpy(sptr,buf,len);
  sptr += len;
  npack++;
}

/* ----------------------------------------------------------------------
   process field rbuf, one field at a time
   return field type
   buf = ptr to field
   len = 1 for BYTE, INT, UINT64, DOUBLE
   len = byte count for RAW and STRING (including NULL)
   len = # of array values for ARRAY types
   PHISH_PICKLE is same as PHISH_RAW
------------------------------------------------------------------------- */

int phish_unpack(char **buf, int32_t *len)
{
  if (nunpack == nrfields) phish_error("Recv buffer empty");

  int32_t type = *(int32_t *) rptr;
  rptr += sizeof(int32_t);

  int32_t nbytes;
  switch (type) {
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
  case PHISH_RAW:
    *len = nbytes = *(int32_t *) rptr;
    rptr += sizeof(int32_t);
    break;
  case PHISH_STRING:
    *len = nbytes = *(int32_t *) rptr;
    rptr += sizeof(int32_t);
    break;
  case PHISH_INT8_ARRAY:
    *len = *(int32_t *) rptr;
    rptr += sizeof(int32_t);
    nbytes = *len * sizeof(int8_t);
    break;
  case PHISH_INT16_ARRAY:
    *len = *(int32_t *) rptr;
    rptr += sizeof(int32_t);
    nbytes = *len * sizeof(int16_t);
    break;
  case PHISH_INT32_ARRAY:
    *len = *(int32_t *) rptr;
    rptr += sizeof(int32_t);
    nbytes = *len * sizeof(int32_t);
    break;
  case PHISH_INT64_ARRAY:
    *len = *(int32_t *) rptr;
    rptr += sizeof(int32_t);
    nbytes = *len * sizeof(int64_t);
    break;
  case PHISH_UINT8_ARRAY:
    *len = *(int32_t *) rptr;
    rptr += sizeof(int32_t);
    nbytes = *len * sizeof(uint8_t);
    break;
  case PHISH_UINT16_ARRAY:
    *len = *(int32_t *) rptr;
    rptr += sizeof(int32_t);
    nbytes = *len * sizeof(uint16_t);
    break;
  case PHISH_UINT32_ARRAY:
    *len = *(int32_t *) rptr;
    rptr += sizeof(int32_t);
    nbytes = *len * sizeof(uint32_t);
    break;
  case PHISH_UINT64_ARRAY:
    *len = *(int32_t *) rptr;
    rptr += sizeof(int32_t);
    nbytes = *len * sizeof(uint64_t);
    break;
  case PHISH_FLOAT_ARRAY:
    *len = *(int32_t *) rptr;
    rptr += sizeof(int32_t);
    nbytes = *len * sizeof(float);
    break;
  case PHISH_DOUBLE_ARRAY:
    *len = *(int32_t *) rptr;
    rptr += sizeof(int32_t);
    nbytes = *len * sizeof(double);
    break;
  case PHISH_PICKLE:
    *len = nbytes = *(int32_t *) rptr;
    rptr += sizeof(int32_t);
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

int phish_datum(int flag)
{
  if (flag == 0) return nrfields;
  if (flag == 1) return lastport;
  phish_error("Invalid flag in phish_datum");
  return 0;
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
   remove datum N from internal queue
   copy it to receive buffer, as if just received, compress queue
   return # of values in the datum
------------------------------------------------------------------------- */

int phish_dequeue(int n)
{
  if (n < 0 || n >= nqueue) phish_error("Invalid index in phish_dequeue");

 nrbytes = queue[n].nbytes;
 memcpy(rbuf,queue[n].datum,nrbytes);
 delete [] queue[n].datum;
 nrfields = (int) (*(int32_t *) rbuf);
 rptr = rbuf + sizeof(int32_t);
 nunpack = 0;
 lastport = queue[n].iport;

 memmove(&queue[n],&queue[n+1],(nqueue-n-1)*sizeof(Queue));
 nqueue--;

 return nrfields;
}

/* ----------------------------------------------------------------------
   return count of internally queued datums
------------------------------------------------------------------------- */

int phish_nqueue()
{
  return nqueue;
}

/* ----------------------------------------------------------------------
   return internal PHISH info
   current keywords are all for minnow counts and input/output ports
------------------------------------------------------------------------- */

int phish_query(const char *keyword, int flag1, int flag2)
{
  phish_assert_initialized();

  if (strcmp(keyword,"idlocal") == 0) return g_local_id;
  else if (strcmp(keyword,"nlocal") == 0) return g_local_count;
  else if (strcmp(keyword,"idglobal") == 0) return g_global_id;
  else if (strcmp(keyword,"nglobal") == 0) return g_global_count;
  else if (strcmp(keyword,"inport/status") == 0) {
    int iport = flag1;
    if (iport < 0 || iport >= MAXPORT) 
      phish_return_error("Invalid phish_query flags", -3);
    return inports[iport].status;
  } else if (strcmp(keyword,"inport/connections") == 0) {
    int iport = flag1;
    if (iport < 0 || iport >= MAXPORT) 
      phish_return_error("Invalid phish_query flags", -3);
    if (inports[iport].status == UNUSED_PORT) return -1;
    return inports[iport].nconnect;
  } else if (strcmp(keyword,"inport/nminnows") == 0) {
    int iport = flag1;
    if (iport < 0 || iport >= MAXPORT) 
      phish_return_error("Invalid phish_query flags", -3);
    if (inports[iport].status == UNUSED_PORT) return -1;
    int iconnect = flag2;
    if (iconnect < 0 || iconnect >= inports[iport].nconnect) return -1;
    return inports[iport].connects[iconnect].nsend;
  } else if (strcmp(keyword,"outport/status") == 0) {
    int iport = flag1;
    if (iport < 0 || iport >= MAXPORT) 
      phish_return_error("Invalid phish_query flags", -3);
    return outports[iport].status;
  } else if (strcmp(keyword,"outport/connections") == 0) {
    int iport = flag1;
    if (iport < 0 || iport >= MAXPORT) 
      phish_return_error("Invalid phish_query flags", -3);
    if (outports[iport].status == UNUSED_PORT) return -1;
    return outports[iport].nconnect;
  } else if (strcmp(keyword,"outport/nminnows") == 0) {
    int iport = flag1;
    if (iport < 0 || iport >= MAXPORT) 
      phish_return_error("Invalid phish_query flags", -3);
    if (outports[iport].status == UNUSED_PORT) return -1;
    int iconnect = flag2;
    if (iconnect < 0 || iconnect >= outports[iport].nconnect) return -1;
    return outports[iport].connects[iconnect].nrecv;
  } else if (strcmp(keyword,"outport/direct") == 0) {
    int iport = flag1;
    if (iport < 0 || iport >= MAXPORT) 
      phish_return_error("Invalid phish_query flags", -3);
    if (outports[iport].status == UNUSED_PORT) return -1;
    for (int iconnect = 0; iconnect < outports[iport].nconnect; iconnect++)
      if (outports[iport].connects[iconnect].style == DIRECT) 
	return outports[iport].connects[iconnect].nrecv;
    return -1;
  } else phish_return_error("Invalid phish_query keyword", -1);

  return 0;
}

/* ----------------------------------------------------------------------
   reset internal PHISH info
   keyword = "ring/receiver"
     reset the receiver proc that I send to on output port iport
     only valid for connection style RING
     used by application to permute the ordering of the ring
------------------------------------------------------------------------- */

int phish_set(const char *keyword, int flag1, int flag2)
{
  phish_assert_initialized();

  if (strcmp(keyword,"ring/receiver") == 0) {
    int iport = flag1;
    int receiver = flag2;

    if (iport < 0 || iport >= MAXPORT) 
      phish_return_error("Invalid port ID in phish_set", -1);
    OutPort *op = &outports[iport];
    if (op->status == UNUSED_PORT || op->status == CLOSED_PORT) 
      phish_return_error("Unused or closed port in phish_set ring/receiver",
                         -1);

    for (int iconnect = 0; iconnect < op->nconnect; iconnect++) {
      OutConnect *oc = &op->connects[iconnect];
      if (oc->style == RING) {
	if (receiver < 0 || receiver >= oc->nrecv)
	  phish_return_error("Invalid receiver in phish_set ring/receiver", -1);
	oc->recvone = oc->recvfirst + receiver;
      }
    }

  } else phish_return_error("Invalid phish_set keyword", -1);

  return 0;
}

void phish_abort()
{
  if(!phish_abort_internal())
    return;

  MPI_Abort(world,1);
}

/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */

/* ----------------------------------------------------------------------
   internal functions, NOT functions in the PHISH API
------------------------------------------------------------------------- */

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
