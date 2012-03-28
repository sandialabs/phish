#!/usr/local/bin/python

# ----------------------------------------------------------------------
#   PHISH = Parallel Harness for Informatic Stream Hashing
#   http://www.sandia.gov/~sjplimp/phish.html
#   Steve Plimpton, sjplimp@sandia.gov, Sandia National Laboratories
#
#   Copyright (2012) Sandia Corporation.  Under the terms of Contract
#   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
#   certain rights in this software.  This software is distributed under 
#   the modified Berkeley Software Distribution (BSD) License.
#
#   See the README file in the top-level PHISH directory.
# -------------------------------------------------------------------------

# bait.py tool for PHISH
# converts a PHISH input script to a MPI or socket launch script

import sys,re,os

# print error message and quit

def error(msg):
  print "ERROR:",msg
  sys.exit()

# extract next command from lines of input script
# return command name and args, delete processed lines from lines
# return "" at end of input script
# allow for blank lines, comment char "#", continuation char "&"
# perform variable replacement

def next_command(lines):
  if not lines: return "",[]
  line = ""
  while not line and lines: 
    line = lines.pop(0).strip()
    index = line.find('#')
    if index >= 0: line = line[:index].rstrip()
  if not line: return "",[]

  while line[-1] == '&':
    line = line[:-1]
    if not lines: error("Command %s is incomplete" % line)
    addline = lines.pop(0).strip()
    if not addline: error("Command %s is incomplete" % line)
    index = addline.find('#')
    if index >= 0: addline = addline[:index].rstrip()
    if not addline: error("Command %s is incomplete" % line)
    line += addline
    print line

  pattern = "(\$[^{])"
  matches = re.findall(pattern,line)
  pattern = "(\$\{.+?\})"
  matches += re.findall(pattern,line)
  for match in matches:
    if len(match) == 2: vname = match[1]
    else: vname = match[2:-1]
    if vname not in variables:
      error("Variable %s does not exist" % vname)
    index = line.index(match)
    line = line[:index] + " ".join(variables[vname]) + line[index+len(match):]

  words = line.split()
  return words[0],words[1:]
  
# set a global setting via command in input script

def set(args):
  global memchunk,safe,port
  if args[0] == "memory":
    if len(args) != 2: error("Illegal set command");
    memchunk = int(args[1])
    if memchunk < 0: error("Illegal set command");
  elif args[0] == "safe":
    if len(args) != 1: error("Illegal set command");
    safe = 1
  else: error("Unrecognized set parameter %s" % arg[0])

# convert non-default global settings to a minnow param string

def set2param():
  str = ""
  if memchunk != MEMCHUNK: str += " -memory %d" % memchunk
  if safe != SAFE: str += " -safe"
  return str

# create a variable via variable command in input script

def variable(args):
  if len(args) < 2: error("Illegal variable command");
  if args[0] in variables: error("Variable %s already in use" % args[0]);
  variables[args[0]] = args[1:]

# MINNOW class instantiated by minnow command in input script

class minnow:
  def __init__(self,args):
    narg = len(args)
    if narg < 2: error("Invalid minnow command")
    self.id = args[0]
    self.exe = args[1]
    self.args = args[2:]
    self.send = []
    self.recv = []
    ids = [minnow.id for minnow in minnows]
    if self.id in ids: error("Minnow ID %s already defined" % self.id)

# CONNECT class instantiated by connect command in input script
# special case handling of first/last arg for publish/subscribe styles
    
class connect:
  def __init__(self,args):
    narg = len(args)
    if narg < 3: error("Invalid connect command")
    self.sender = args[0]
    if ':' in self.sender and args[1] != "subscribe":
      self.sender,self.sendport = self.sender.split(':',1)
      self.sendport = int(self.sendport)
    else: self.sendport = 0
    self.style = args[1]
    self.receiver = args[2]
    if ':' in self.receiver:
      self.receiver,self.recvport = self.receiver.split(':',1)
      self.recvport = int(self.recvport)
    else: self.recvport = 0

    if self.style == "publish":
      self.socket = self.receiver
      self.receiver = ""
      self.recvport = -1
    if self.style == "subscribe":
      self.socket = self.sender
      self.sender = ""
      self.sendport = -1
      
    ids = [minnow.id for minnow in minnows]
    if self.sender and self.sender not in ids:
      error("Unrecognized connect ID %s" % self.sender)
    if self.receiver and self.receiver not in ids:
      error("Unrecognized connect ID %s" % self.receiver)
    
# LAYOUT class instantiated by layout command in input script

class layout:
  def __init__(self,args):
    narg = len(args)
    if narg < 2: error("Invalid layout command")
    self.id = args[0]
    self.nprocs = int(args[1])
    ids = [layout.id for layout in layouts]
    if self.id in ids: error("Layout ID %s already defined" % self.id)
    
    self.invoke = None
    self.host = None
    iarg = 2
    while iarg < narg:
      if args[iarg] == "invoke":
        if iarg+2 > narg: error("Invalid layout command")
        self.invoke = args[iarg+1]
        iarg += 2
      elif args[iarg] == "host":
        if iarg+2 > narg: error("Invalid layout command")
        self.host = args[iarg+1]
        iarg += 2
      else: error("Invalid layout command")
        
# mode-dependent output
# sendport and recvport are strings which can encode more info than an int

def output_mpich():
  fp = open(outfile,"w")
  
  for iminnow,minnow in enumerate(minnows):
    procstr = "-n %d" % (minnow.nprocs)
    
    if minnow.invoke: exestr = " %s %s" % (minnow.invoke,minnow.pathexe)
    else: exestr = " %s" % minnow.pathexe
    
    minnowstr = " -minnow %s %s %d %d" % \
        (minnow.exe,minnow.id,minnow.nprocs,minnow.procstart)

    paramstr = set2param()
    
    instr = ""
    for recv in minnow.recv:
      if recv[0] >= 0:
        nprocs_send = minnows[recv[0]].nprocs
        procstart_send = minnows[recv[0]].procstart
      else: nprocs_send = procstart_send = -1
      if recv[2] >= 0:
        nprocs_recv = minnows[recv[2]].nprocs
        procstart_recv = minnows[recv[2]].procstart
      else: nprocs_recv = procstart_recv = -1
      style = connects[recv[1]].style
      if style == "subscribe": style += "/" + connects[recv[1]].socket
      instr += " -in %d %d %d %s %d %d %d" % \
          (nprocs_send,procstart_send,connects[recv[1]].sendport,
           style,
           nprocs_recv,procstart_recv,connects[recv[1]].recvport)
          
    outstr = ""
    for send in minnow.send:
      if send[0] >= 0:
        nprocs_send = minnows[send[0]].nprocs
        procstart_send = minnows[send[0]].procstart
      else: nprocs_send = procstart_send = -1
      if send[2] >= 0:
        nprocs_recv = minnows[send[2]].nprocs
        procstart_recv = minnows[send[2]].procstart
      else: nprocs_recv = procstart_recv = -1
      style = connects[send[1]].style
      if style == "publish": style += "/" + connects[send[1]].socket
      outstr += " -out %d %d %d %s %d %d %d" % \
          (nprocs_send,procstart_send,connects[send[1]].sendport,
           style,
           nprocs_recv,procstart_recv,connects[send[1]].recvport)

    launchstr = procstr + exestr + minnowstr + paramstr + instr + outstr
    if minnow.args: launchstr += " -args " + " ".join(minnow.args)
    # should just need following line, but MPICH has a configfile bug
    # print >>fp,launchstr
    print >>fp,launchstr,
    if iminnow < len(minnows)-1: print >>fp,":"
    else: print >>fp
  fp.close()

def output_openmpi():
  fp = open("outfile","w")
  print >>fp,"mpirun",

  for iminnow,minnow in enumerate(minnows):
    procstr = "-n %d %s" % (minnow.nprocs,minnow.pathexe)
    
    minnowstr = " -minnow %s %s %d %d" % \
        (minnow.exe,minnow.id,minnow.nprocs,minnow.procstart)

    paramstr = set2param()

    instr = ""
    for recv in minnow.recv:
      if recv[0] >= 0:
        nprocs_send = minnows[recv[0]].nprocs
        procstart_send = minnows[recv[0]].procstart,
      else: nprocs_send = procstart_send = -1
      if recv[2] >= 0:
        nprocs_recv = minnows[recv[2]].nprocs
        procstart_recv = minnows[recv[2]].procstart,
      else: nprocs_recv = procstart_recv = -1
      style = connects[recv[1]].style
      if style == "subscribe": style += "/" + connects[recv[1]].socket
      instr += " -in %d %d %d %s %d %d %d" % \
          (nprocs_send,procstart_send,connects[recv[1]].sendport,
           style,
           nprocs_recv,procstart_recv,connects[recv[1]].recvport)

    outstr = ""
    for send in minnow.send:
      if send[0] >= 0:
        nprocs_send = minnows[send[0]].nprocs
        procstart_send = minnows[send[0]].procstart,
      else: nprocs_send = procstart_send = -1
      if send[2] >= 0:
        nprocs_recv = minnows[send[2]].nprocs
        procstart_recv = minnows[send[2]].procstart,
      else: nprocs_recv = procstart_recv = -1
      style = connects[send[1]].style
      if style == "publish": style += "/" + connects[send[1]].socket
      outstr += " -out %d %d %d %s %d %d %d" % \
          (nprocs_send,procstart_send,connects[send[1]].sendport,
           style,
           nprocs_recv,procstart_recv,connects[send[1]].recvport)
          
    launchstr = procstr + minnowstr + paramstr + instr + outstr
    if minnow.args: launchstr += " -args " + " ".join(minnow.args)
    print >>fp,launchstr,
    if iminnow < len(minnows)-1: print >>fp,":",
    else: print >>fp
  fp.close()

def output_socket():
  return

# ---------------------------------------------------------------------------
# MAIN program

from version import version
print "PHISH version:",version

# parse command-line args to override default settings

narg = len(sys.argv)
args = sys.argv

variables = {}
outfile = "outfile"
paths = [""]
mode = "mpich"

iarg = 1
while iarg < narg:
  if args[iarg] == "-var" or args[iarg] == "-v":
    if iarg+3 > narg: error("Invalid command line args")
    start = stop = iarg+2
    while stop < narg and args[stop][0] != '-': stop += 1
    variables[args[iarg+1]] = args[start:stop]
    iarg += 2+stop-start
  elif args[iarg] == "-out" or args[iarg] == "-o":
    if iarg+2 > narg: error("Invalid command line args")
    outfile = args[iarg+1]
    iarg += 2
  elif args[iarg] == "-path" or args[iarg] == "-p":
    if iarg+2 > narg: error("Invalid command line args")
    paths += args[iarg+1].split(':')
    iarg += 2
  elif args[iarg] == "-mode" or args[iarg] == "-m":
    if iarg+2 > narg: error("Invalid command line args")
    mode = args[iarg+1]
    if mode != "mpich" and mode != "openmpi" and mode != "socket":
      error("Invalid command line args");
    iarg += 2
  else: error("Invalid command line args")

if mode == "socket": error("Socket mode not yet supported")

# defaults for variables specfied by set command

memchunk = MEMCHUNK = 1
safe = SAFE = 0

# initialize data structures for minnows, connects, layouts

minnows = []
connects = []
layouts = []

# read and process the input script

lines = sys.stdin.readlines()
while lines:
  command,args = next_command(lines)
  if not command: break
  elif command == "set": set(args)
  elif command == "variable": variable(args)
  elif command == "minnow": minnows.append(minnow(args))
  elif command == "connect": connects.append(connect(args))
  elif command == "layout": layouts.append(layout(args))
  else: error("Unrecognized command %s" % command)

# add fields to each minnow based on layout and connect params
# check that connections are consistent with layout
  
minnowids = [minnow.id for minnow in minnows]
layoutids = [layout.id for layout in layouts]

nprocs = 0
for minnow in minnows:
  if minnow.id not in layoutids: minnow.nprocs = 1
  else:
    index = layoutids.index(minnow.id)
    minnow.nprocs = layouts[index].nprocs
    minnow.invoke = layouts[index].invoke
  minnow.procstart = nprocs
  nprocs += minnow.nprocs
  
for iconnect,connect in enumerate(connects):
  if connect.sender: sendindex = minnowids.index(connect.sender)
  else: sendindex = -1
  if connect.receiver: recvindex = minnowids.index(connect.receiver)
  else: recvindex = -1

  if sendindex >= 0:
    minnows[sendindex].send.append([sendindex,iconnect,recvindex])
  if recvindex >= 0:
    minnows[recvindex].recv.append([sendindex,iconnect,recvindex])

  if sendindex >= 0:
    npsend = minnows[sendindex].nprocs
    sid = minnows[sendindex].id
  if recvindex >= 0:
    nprecv = minnows[recvindex].nprocs
    rid = minnows[recvindex].id
  
  if connect.style == "single":
    if nprecv != 1:
      error("Invalid connection between %s and %s" % (sid,rid));
  elif connect.style == "paired":
    if npsend != nprecv:
      error("Invalid connection between %s and %s" % (sid,rid));
  elif connect.style == "hashed":
    continue
  elif connect.style == "roundrobin":
    continue
  elif connect.style == "direct":
    continue
  elif connect.style == "bcast":
    continue
  elif connect.style == "chain":
    if sid != rid or npsend == 1:
      error("Invalid connection between %s and %s" % (sid,rid));
  elif connect.style == "ring":
    if sid != rid or npsend == 1:
      error("Invalid connection between %s and %s" % (sid,rid));
  elif connect.style == "publish":
    continue
  elif connect.style == "subscribe":
    continue
  else:
    error("Unrecognized connect style %s" % connect.style);

# generate full executable names using list of paths

for minnow in minnows:
  flag = 0
  for path in paths:
    if not path: pathexe = minnow.exe
    else: pathexe = path + '/' + minnow.exe
    if os.path.isfile(pathexe):
      if not os.access(pathexe,os.X_OK):
        error("Minnow %s is not executable" % pathexe)
      minnow.pathexe = pathexe
      flag = 1
      break
  if not flag: error("Minnow %s could not be found in path list" % minnow.exe)

# create output depending on mode

if mode == "mpich": output_mpich()
elif mode == "openmpi": output_openmpi()
elif mode == "socket": output_socket()

# print stats

print "# of minnows =",len(minnows)
print "# of processes =",nprocs

# tell user for how to invoke launch script

if mode == "mpich":
  print "MPICH: mpiexec -configfile %s" % outfile
elif mode == "openmpi":
  print "OpenMPI: invoke %s from shell" % outfile
elif mode == "socket":
  print "Sockets: invoke %s from shell" % outfile
