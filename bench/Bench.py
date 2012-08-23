#!/usr/local/bin/python

# benchmark various PHISH scripts
# some runs below are OpenMPI specific with rankfile

# Syntax: python Bench.py logfile [pp] [chain] [hash]
#                                 [mpi] [pympi] [zmq] [pyzmq] [mpionly] 
#         pp,chain,hash = which benchmarks to run
#         mpi,pympi,zmq,pyzmq,mpionly = which backends to run
#                                       mpionly = non-PHISH MPI-only

import sys,commands,re

# which Python to use

# desktop
#python = "python"
# RedSky
python = "/ascldap/users/tshead/install/python/bin/python2.7"

# settings for machine and looping and timing and different test sizes

numnode = 32       # allocated # of nodes
pernode = 8        # of cores per node
rankfileflag = 1   # 1 if supports OpenMPI rankfiles

#hostnames = [commands.getoutput("hostname")]
hostnames = commands.getoutput("scontrol show hostnames").split()

safe = 10000       # applied to chain and hash

mincpu = 30.0      # minimum CPU seconds
miniter = 100000   # minimum iterations
increase = 2       # increase iterations by this factor every time
repeat = 2         # repeat final run that exceeds mincpu this many times

ppsizes = [0,64,256,1024,4096,16384]

chainprocs = [2,4,8,16,32,64,128,256]
chainsizes = [0,64,256,1024,4096,16384]

hashprocs = [2,4,8,16,32,64,128,256]
hashsizes = [0,64,256,1024,4096,16384]

# -------------------------------------------------------------

def output(str):
  print str
  print >>log,str

def size2kbytes(bytes):
  kbytes = (bytes+32)/1024
  if (bytes+32) % 1024: kbytes += 1
  return kbytes

def extract_cpu(str):
  match = re.search("= (.*?) secs",str)
  if not match: return None
  return float(match.group(1))

# -------------------------------------------------------------

# run ping-pong test on 2 procs for different messages sizes
# 1st run = 2 cores on same node
# 2nd run = one core on each of 2 nodes, via OpenMPI rankfile

def pp(which):
  
  for size in ppsizes:
    kbytes = size2kbytes(size)
    
    if which == "mpionly":
      masterstr = "mpirun -np 2 pingpong %%d %d" % size
    else:
      if which == "mpi":
        str = "%s ../bait/bait.py -p ../minnow -b mpi-config " + \
            "-s memory %d -v N %%d -v M %d < ../example/in.pp"
        str = str % (python,kbytes,size)
      elif which == "pympi":
        str = "%s ../bait/bait.py -p ../minnow -b mpi-config " + \
            "-s memory %d -v N %%d -v M %d -x .py -l %s < ../example/in.pp"
        str = str % (python,kbytes,size,python)
      elif which == "zmq":
        str = "%s ../bait/bait.py -p ../minnow -b zmq " + \
            "-s memory %d -v N %%d -v M %d -v hostnames %s < ../example/in.pp"
        str = str % (python,kbytes,size," ".join(hostnames))
      elif which == "pyzmq":
        str = "%s ../bait/bait.py -p ../minnow -b zmq " + \
            "-s memory %d -v N %%d -v M %d -v hostnames %s " + \
            "-x .py -l %s < ../example/in.pp"
        str = str % (python,kbytes,size," ".join(hostnames),python)

      out = commands.getoutput(str)
      lines = out.split('\n')
      str = ' '.join(lines[2:])
      masterstr = "mpirun %s" % str
      
    iter = miniter
    while 1:
      runstr = masterstr % iter
      output("Running PP.%s on 2 procs with N = %d, M = %d" % (which,iter,size))
      output(runstr)
      out = commands.getoutput(runstr)
      output(out + "\n\n")
      cpu = extract_cpu(out)
      if not cpu: break
      if cpu < mincpu: iter *= increase
      else: break
      
    for i in range(repeat-1):
      output("Running PP.%s on 2 procs with N = %d, M = %d" % (which,iter,size))
      output(runstr)
      out = commands.getoutput(runstr)
      output(out + "\n\n")

  if not rankfileflag: return
      
  for size in ppsizes:
    kbytes = size2kbytes(size)

    if which == "mpionly":
      fp = open("rankfile","w")
      print >>fp,"rank 0=+n0 slot=0"
      print >>fp,"rank 1=+n1 slot=0"
      fp.close()
      masterstr = "mpirun -rf rankfile -np 2 pingpong %%d %d" % size
    else:
      if which == "mpi":
        str = "%s ../bait/bait.py -p ../minnow -b mpi-config " + \
            "-s memory %d -s numnode 2 -s bindorder 2 " + \
            "-v N %%d -v M %d < ../example/in.pp"
        str = str % (python,kbytes,size)
      elif which == "pympi":
        str = "%s ../bait/bait.py -p ../minnow -b mpi-config " + \
            "-s memory %d -s numnode 2 -s bindorder 2 " + \
            "-v N %%d -v M %d -x .py -l %s < ../example/in.pp"
        str = str % (python,kbytes,size,python)
      elif which == "zmq":
        str = "%s ../bait/bait.py -p ../minnow -b zmq " + \
            "-s memory %d -s numnode 2 -s bindorder 2 " + \
            "-v N %%d -v M %d -v hostnames %s < ../example/in.pp"
        str = str % (python,kbytes,size," ".join(hostnames))
      elif which == "pyzmq":
        str = "%s ../bait/bait.py -p ../minnow -b zmq " + \
            "-s memory %d -s numnode 2 -s bindorder 2 " + \
            "-v N %%d -v M %d -v hostnames %s " + \
            "-x .py -l %s < ../example/in.pp"
        str = str % (python,kbytes,size," ".join(hostnames),python)

      out = commands.getoutput(str)
      lines = out.split('\n')
      str = ' '.join(lines[2:])
      masterstr = "mpirun -rf rankfile %s" % str
    
    iter = miniter
    while 1:
      runstr = masterstr % iter
      output("Running PP.%s on 2 procs with N = %d, M = %d" % (which,iter,size))
      output(runstr)
      out = commands.getoutput(runstr)
      output(out + "\n\n")
      cpu = extract_cpu(out)
      if not cpu: break
      if cpu < mincpu: iter *= increase
      else: break
      
    for i in range(repeat-1):
      output("Running PP.%s on 2 procs with N = %d, M = %d" % (which,iter,size))
      output(runstr)
      out = commands.getoutput(runstr)
      output(out + "\n\n")

# -------------------------------------------------------------

# run chain test in double loop over number of procs and messages sizes
# 1st run = core assignment loops over cores, then over used nodes
# 2nd run = core assignment loops over used nodes, then over cores
# 3rd run = core assignment loops over allocated nodes, then over cores

def chain(which):
  
  for nprocs in chainprocs:
    for size in chainsizes:
      kbytes = size2kbytes(size)

      if which == "mpionly":
        masterstr = "mpirun -np %d chain %%d %d %d" % (nprocs,size,safe)
      else:
        if which == "mpi":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d -v P %d -v N %%d -v M %d < ../example/in.chain"
          str = str % (python,safe,kbytes,nprocs,size)
        elif which == "pympi":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d -v P %d -v N %%d -v M %d -x .py -l %s < " + \
              "../example/in.chain"
          str = str % (python,safe,kbytes,nprocs,size,python)
        elif which == "zmq":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d -v P %d -v N %%d -v M %d -v hostnames %s " + \
              "< ../example/in.chain"
          str = str % (python,safe,kbytes,nprocs,size," ".join(hostnames))
        elif which == "pyzmq":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d -v P %d -v N %%d -v M %d -v hostnames %s " + \
              "-x .py -l %s < ../example/in.chain"
          str = str % (python,safe,kbytes,nprocs,size,
                       " ".join(hostnames),python)
        
        out = commands.getoutput(str)
        lines = out.split('\n')
        str = ' '.join(lines[2:])
        masterstr = "mpirun %s" % str
    
      iter = miniter
      while 1:
        runstr = masterstr % iter
        output("Running CHAIN.%s on %d procs with N = %d, M = %d" %
               (which,nprocs,iter,size))
        output(runstr)
        out = commands.getoutput(runstr)
        output(out + "\n\n")
        cpu = extract_cpu(out)
        if not cpu: break
        if cpu < mincpu: iter *= increase
        else: break
      
      for i in range(repeat-1):
        output("Running CHAIN.%s on %d procs with N = %d, M = %d" %
               (which,nprocs,iter,size))
        output(runstr)
        out = commands.getoutput(runstr)
        output(out + "\n\n")

  if not rankfileflag: return

  for nprocs in chainprocs:
    for size in chainsizes:
      kbytes = size2kbytes(size)

      if which == "mpionly":
        fp = open("rankfile","w")
        num = nprocs / pernode
        if nprocs & pernode: num += 1
        iproc = 0
        for icore in range(pernode):
          for inode in range(num):
            if iproc < nprocs:
              print >>fp,"rank %d=+n%d slot=%d" % (iproc,inode,icore)
            iproc += 1
        fp.close()
        masterstr = "mpirun -rf rankfile -np %d chain %%d %d %d" % \
            (nprocs,size,safe)
      else:
        if which == "mpi":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d -s pernode %d -s bindorder 2 " + \
              "-v P %d -v N %%d -v M %d < ../example/in.chain"
          str = str % (python,safe,kbytes,pernode,nprocs,size)
        elif which == "pympi":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d -s pernode %d -s bindorder 2 " + \
              "-v P %d -v N %%d -v M %d -x .py -l %s < " + \
              "../example/in.chain"
          str = str % (python,safe,kbytes,pernode,nprocs,size,python)
        elif which == "zmq":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d -s pernode %d -s bindorder 2 " + \
              "-v P %d -v N %%d -v M %d -v hostnames %s " + \
              "< ../example/in.chain"
          str = str % (python,safe,kbytes,pernode,
                       nprocs,size," ".join(hostnames))
        elif which == "pyzmq":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d -s pernode %d -s bindorder 2 " + \
              "-v P %d -v N %%d -v M %d -v hostnames %s " + \
              "-x .py -l %s < ../example/in.chain"
          str = str % (python,safe,kbytes,pernode,
                       nprocs,size," ".join(hostnames),python)

        out = commands.getoutput(str)
        lines = out.split('\n')
        str = ' '.join(lines[2:])
        masterstr = "mpirun -rf rankfile %s" % str

      iter = miniter
      while 1:
        runstr = masterstr % iter
        output("Running CHAIN.%s on %d procs with N = %d, M = %d" %
               (which,nprocs,iter,size))
        output(runstr)
        out = commands.getoutput(runstr)
        output(out + "\n\n")
        cpu = extract_cpu(out)
        if not cpu: break
        if cpu < mincpu: iter *= increase
        else: break
      
      for i in range(repeat-1):
        output("Running CHAIN.%s on %d procs with N = %d, M = %d" %
               (which,nprocs,iter,size))
        output(runstr)
        out = commands.getoutput(runstr)
        output(out + "\n\n")

  for nprocs in chainprocs:
    for size in chainsizes:
      kbytes = size2kbytes(size)

      if which == "mpionly":
        fp = open("rankfile","w")
        iproc = 0
        for icore in range(pernode):
          for inode in range(numnode):
            if iproc < nprocs:
              print >>fp,"rank %d=+n%d slot=%d" % (iproc,inode,icore)
            iproc += 1
        fp.close()
        masterstr = "mpirun -rf rankfile -np %d chain %%d %d %d" % \
            (nprocs,size,safe)
      else:
        if which == "mpi":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d -s numnode %d -s pernode %d -s bindorder 2 " + \
              "-v P %d -v N %%d -v M %d < ../example/in.chain"
          str = str % (python,safe,kbytes,numnode,pernode,nprocs,size)
        elif which == "pympi":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d -s numnode %d -s pernode %d -s bindorder 2 " + \
              "-v P %d -v N %%d -v M %d -x .py -l %s < " + \
              "../example/in.chain"
          str = str % (python,safe,kbytes,numnode,pernode,nprocs,size,python)
        elif which == "zmq":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d -s numnode %d -s pernode %d -s bindorder 2 " + \
              "-v P %d -v N %%d -v M %d -v hostnames %s " + \
              "< ../example/in.chain"
          str = str % (python,safe,kbytes,numnode,pernode,
                       nprocs,size," ".join(hostnames))
        elif which == "pyzmq":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d -s numnode %d -s pernode %d -s bindorder 2 " + \
              "-v P %d -v N %%d -v M %d -v hostnames %s " + \
              "-x .py -l %s < ../example/in.chain"
          str = str % (python,safe,kbytes,numnode,pernode,
                       nprocs,size," ".join(hostnames),python)

        out = commands.getoutput(str)
        lines = out.split('\n')
        str = ' '.join(lines[2:])
        masterstr = "mpirun -rf rankfile %s" % str

      iter = miniter
      while 1:
        runstr = masterstr % iter
        output("Running CHAIN.%s on %d procs with N = %d, M = %d" %
               (which,nprocs,iter,size))
        output(runstr)
        out = commands.getoutput(runstr)
        output(out + "\n\n")
        cpu = extract_cpu(out)
        if not cpu: break
        if cpu < mincpu: iter *= increase
        else: break
      
      for i in range(repeat-1):
        output("Running CHAIN.%s on %d procs with N = %d, M = %d" %
               (which,nprocs,iter,size))
        output(runstr)
        out = commands.getoutput(runstr)
        output(out + "\n\n")

# -------------------------------------------------------------

# run hash test in double loop over number of procs and messages sizes
# first half of procs are sender, 2nd half are receiver
# 1st run = core assignment loops over cores, then over used nodes
# 2nd run = core assignment loops over used nodes, then over cores
# 3rd run = core assignment loops over allocated nodes, then over cores

def hash(which):

  for nprocs in hashprocs:
    nprocs1 = nprocs / 2
    nprocs2 = nprocs - nprocs1
    for size in hashsizes:
      kbytes = size2kbytes(size)

      if which == "mpionly":
        masterstr = "mpirun -np %d hash %%d %d %d" % (nprocs,size,safe)
      else:
        if which == "mpi":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d -v S %d -v R %d -v N %%d -v M %d " + \
              "< ../example/in.hash"
          str = str % (python,safe,kbytes,nprocs1,nprocs2,size)
        elif which == "pympi":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d -v S %d -v R %d -v N %%d -v M %d " + \
              "-x .py -l %s < ../example/in.hash"
          str = str % (python,safe,kbytes,nprocs1,nprocs2,size,python)
        elif which == "zmq":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d " + \
              "-v S %d -v R %d -v N %%d -v M %d -v hostnames %s " + \
              "< ../example/in.hash"
          str = str % (python,safe,kbytes,nprocs1,nprocs2,size,
                       " ".join(hostnames))
        elif which == "pyzmq":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d -v S %d -v R %d -v N %%d -v M %d " + \
              "-v hostnames %s -x .py -l %s < ../example/in.hash"
          str = str % (python,safe,kbytes,nprocs1,nprocs2,size,
                       " ".join(hostnames),python)
          
        out = commands.getoutput(str)
        lines = out.split('\n')
        str = ' '.join(lines[2:])
        masterstr = "mpirun %s" % str
    
      iter = miniter
      while 1:
        runstr = masterstr % iter
        output("Running HASH.%s on %d procs with N = %d, M = %d" %
               (which,nprocs,iter,size))
        output(runstr)
        out = commands.getoutput(runstr)
        output(out + "\n\n")
        cpu = extract_cpu(out)
        if not cpu: break
        if cpu < mincpu: iter *= increase
        else: break
      
      for i in range(repeat-1):
        output("Running HASH.%s on %d procs with N = %d, M = %d" %
               (which,nprocs,iter,size))
        output(runstr)
        out = commands.getoutput(runstr)
        output(out + "\n\n")

  if not rankfileflag: return
        
  for nprocs in hashprocs:
    nprocs1 = nprocs / 2
    nprocs2 = nprocs - nprocs1
    for size in hashsizes:
      kbytes = size2kbytes(size)
      
      if which == "mpionly":
        fp = open("rankfile","w")
        num = nprocs / pernode
        if nprocs & pernode: num += 1
        iproc = 0
        for icore in range(pernode):
          for inode in range(num):
            if iproc < nprocs:
              print >>fp,"rank %d=+n%d slot=%d" % (iproc,inode,icore)
            iproc += 1
        fp.close()
        masterstr = "mpirun -rf rankfile -np %d hash %%d %d %d" % \
            (nprocs,size,safe)
      else:
        if which == "mpi":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d -s pernode %d -s bindorder 2 " + \
              "-v S %d -v R %d -v N %%d -v M %d < ../example/in.hash"
          str = str % (python,safe,kbytes,pernode,nprocs1,nprocs2,size)
        elif which == "pympi":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d -s pernode %d -s bindorder 2 " + \
              "-v S %d -v R %d -v N %%d " + \
              "-v M %d -x .py -l %s < ../example/in.hash"
          str = str % (python,safe,kbytes,pernode,nprocs1,nprocs2,size,python)
        elif which == "zmq":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d -s pernode %d -s bindorder 2 " + \
              "-v S %d -v R %d -v N %%d -v M %d -v hostnames %s " + \
              "< ../example/in.hash"
          str = str % (python,safe,kbytes,pernode,nprocs1,nprocs2,size,
                       " ".join(hostnames))
        elif which == "pyzmq":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d -s pernode %d -s bindorder 2 " + \
              "-v S %d -v R %d -v N %%d -v M %d " + \
              "-v hostnames %s -x .py -l %s < ../example/in.hash"
          str = str % (python,safe,kbytes,pernode,nprocs1,nprocs2,size,
                       " ".join(hostnames),python)

        out = commands.getoutput(str)
        lines = out.split('\n')
        str = ' '.join(lines[2:])
        masterstr = "mpirun -rf rankfile %s" % str

      iter = miniter
      while 1:
        runstr = masterstr % iter
        output("Running HASH.%s on %d procs with N = %d, M = %d" %
               (which,nprocs,iter,size))
        output(runstr)
        out = commands.getoutput(runstr)
        output(out + "\n\n")
        cpu = extract_cpu(out)
        if not cpu: break
        if cpu < mincpu: iter *= increase
        else: break
      
      for i in range(repeat-1):
        output("Running HASH.%s on %d procs with N = %d, M = %d" %
               (which,nprocs,iter,size))
        output(runstr)
        out = commands.getoutput(runstr)
        output(out + "\n\n")

  for nprocs in hashprocs:
    nprocs1 = nprocs / 2
    nprocs2 = nprocs - nprocs1
    for size in hashsizes:
      kbytes = size2kbytes(size)
      
      if which == "mpionly":
        fp = open("rankfile","w")
        iproc = 0
        for icore in range(pernode):
          for inode in range(numnode):
            if iproc < nprocs:
              print >>fp,"rank %d=+n%d slot=%d" % (iproc,inode,icore)
            iproc += 1
        fp.close()
        masterstr = "mpirun -rf rankfile -np %d hash %%d %d %d" % \
            (nprocs,size,safe)
      else:
        if which == "mpi":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d -s numnode %d -s pernode %d -s bindorder 2 " + \
              "-v S %d -v R %d -v N %%d -v M %d < ../example/in.hash"
          str = str % (python,safe,kbytes,numnode,pernode,
                       nprocs1,nprocs2,size)
        elif which == "pympi":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d -s numnode %d -s pernode %d -s bindorder 2 " + \
              "-v S %d -v R %d -v N %%d " + \
              "-v M %d -x .py -l %s < ../example/in.hash"
          str = str % (python,safe,kbytes,numnode,pernode,
                       nprocs1,nprocs2,size,python)
        elif which == "zmq":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d -s numnode %d -s pernode %d -s bindorder 2 " + \
              "-v S %d -v R %d -v N %%d -v M %d -v hostnames %s " + \
              "< ../example/in.hash"
          str = str % (python,safe,kbytes,numnode,pernode,nprocs1,nprocs2,size,
                       " ".join(hostnames))
        elif which == "pyzmq":
          str = "%s ../bait/bait.py -p ../minnow -b mpi-config -s safe %d " + \
              "-s memory %d -s numnode %d -s pernode %d -s bindorder 2 " + \
              "-v S %d -v R %d -v N %%d -v M %d " + \
              "-v hostnames %s -x .py -l %s < ../example/in.hash"
          str = str % (python,safe,kbytes,numnode,pernode,nprocs1,nprocs2,size,
                       " ".join(hostnames),python)

        out = commands.getoutput(str)
        lines = out.split('\n')
        str = ' '.join(lines[2:])
        masterstr = "mpirun -rf rankfile %s" % str

      iter = miniter
      while 1:
        runstr = masterstr % iter
        output("Running HASH.%s on %d procs with N = %d, M = %d" %
               (which,nprocs,iter,size))
        output(runstr)
        out = commands.getoutput(runstr)
        output(out + "\n\n")
        cpu = extract_cpu(out)
        if not cpu: break
        if cpu < mincpu: iter *= increase
        else: break
      
      for i in range(repeat-1):
        output("Running HASH.%s on %d procs with N = %d, M = %d" %
               (which,nprocs,iter,size))
        output(runstr)
        out = commands.getoutput(runstr)
        output(out + "\n\n")

# -------------------------------------------------------------
# -------------------------------------------------------------

# main program

if len(sys.argv) < 3:
  print "Syntax: python Bench.py logfile [pp] [chain] [hash]", \
      "[mpi] [pympi] [zmq] [pyzmq] [mpionly]"
  sys.exit()

log = open(sys.argv[1],"w")

ppflag = chainflag = hashflag = 0
mpiflag = pympiflag = zmqflag = pyzmqflag = mpionlyflag = 0

for arg in sys.argv[2:]:
  if arg == "pp": ppflag = 1
  elif arg == "chain": chainflag = 1
  elif arg == "hash": hashflag = 1
  elif arg == "mpi": mpiflag = 1
  elif arg == "pympi": pympiflag = 1
  elif arg == "zmq": zmqflag = 1
  elif arg == "pyzmq": pyzmqflag = 1
  elif arg == "mpionly": mpionlyflag = 1
  else: 
    print "Syntax: python Bench.py logfile [pp] [chain] [hash]", \
        "[mpi] [pympi] [zmq] [pyzmq] [mpionly]"
    sys.exit()
    
# perform selected benchmark runs

if ppflag:
  if mpiflag: pp("mpi")
  if zmqflag: pp("zmq")
  if pympiflag: pp("pympi")
  if pyzmqflag: pp("pyzmq")
  if mpionlyflag: pp("mpionly")

if chainflag:
  if mpiflag: chain("mpi")
  if zmqflag: chain("zmq")
  if pympiflag: chain("pympi")
  if pyzmqflag: chain("pyzmq")
  if mpionlyflag: chain("mpionly")

if hashflag:
  if mpiflag: hash("mpi")
  if zmqflag: hash("zmq")
  if pympiflag: hash("pympi")
  if pyzmqflag: hash("pyzmq")
  if mpionlyflag: hash("mpionly")

# close log file
    
log.close()
