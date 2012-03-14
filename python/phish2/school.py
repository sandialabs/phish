from __future__ import absolute_import

import json
import subprocess
import sys
import traceback
import zmq

_debug = False
_names = []
_hosts = []
_arguments = []
_connections = []

ROUND_ROBIN = "round-robin"
HASHED = "hashed"
BROADCAST = "broadcast"

def reset():
  global _names, _hosts, _arguments, _connections

  _names = []
  _hosts = []
  _arguments = []
  _connections = []

def debug(enable):
  global _debug
  _debug = enable

class local_process:
  def __init__(self, name, rank, arguments):
    self.name = name
    self.rank = rank
    if _debug:
      self.process = subprocess.Popen(arguments)
    else:
      self.process = subprocess.Popen(arguments, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

  def wait(self):
    stdout, stderr = self.process.communicate()
    if stdout is not None and stderr is not None:
      sys.stderr.write("\n==> localhost: %s %s <==\n" % (self.rank, self.name))
      sys.stdout.write(stdout)
      sys.stderr.write(stderr)

  def kill(self):
    if self.process.returncode is None:
      self.process.terminate()

class ssh_process:
  def __init__(self, name, rank, arguments, host):
    self.name = name
    self.rank = rank
    self.host = host
    ssh_arguments = ["ssh", "-x", host]
    ssh_arguments += arguments
    #print " ".join(ssh_arguments)
    self.process = subprocess.Popen(ssh_arguments, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

  def wait(self):
    stdout, stderr = self.process.communicate()
    sys.stderr.write("\n==> %s: %s %s <==\n" % (self.host, self.rank, self.name))
    sys.stdout.write(stdout)
    sys.stderr.write(stderr)

  def kill(self):
    if self.process.returncode is None:
      self.process.terminate()

def create_minnows(name, arguments, count=1, hosts=None):
  """Starts one-or-more minnow processes, using the given command-line
  arguments, count of processes to create, and optional list of hosts where
  processes should be run, returning a list containing handles to the new
  minnows.  The hosts parameter should be None (all minnows will be started on
  the localhost) a string hostname (all minnows will be started on the given
  host) or a collection of string hostnames, one-per-minnow."""
  global _names
  global _hosts
  global _arguments

  results = range(len(_names), len(_names) + count)
  _names += [name] * count
  _arguments += [arguments] * count

  if hosts is None:
    _hosts += ["127.0.0.1"] * count
  elif isinstance(hosts, str):
    _hosts += [hosts] * count
  else:
    if len(hosts) != count:
      raise Exception("Host count must match minnow count.")
    _hosts += hosts

  return results

def all_to_all(output_minnows, output_port, send_pattern, input_port, input_minnows):
  """Creates all-to-all connections from the output ports on one set of
  minnows to the input ports of another."""
  for output_minnow in output_minnows:
    _connections.append((output_minnow, output_port, send_pattern, input_port, input_minnows))

def one_to_one(output_minnows, output_port, input_port, input_minnows):
  """Connects an output port from one minnow to the input port of the
  corresponding minnow, for two equal sets of minnows."""
  if len(output_minnows) != len(input_minnows):
    raise Exception("Output and input process counts must be the same.")

  for i in range(len(output_minnows)):
    _connections.append((output_minnows[i], output_port, BROADCAST, input_port, input_minnows[i:i+1]))

def start():
  """Notifies the school that all minnows and connections have been specified, and
  processing may begin.  This call will not return until all minnows have exited."""
  # Assign addresses to control ports ...
  next_port = 5555
  control_ports_internal = []
  control_ports_external = []
  for i in range(len(_names)):
    control_ports_internal.append("tcp://*:%s" % (next_port))
    control_ports_external.append("tcp://%s:%s" % (_hosts[i], next_port))
    next_port += 1

  #if _debug:
  #  print "internal control ports: %s" % control_ports_internal
  #  print "external control ports: %s" % control_ports_external

  # Assign addresses to input ports ...
  input_ports_internal = []
  input_ports_external = []
  for i in range(len(_names)):
    input_ports_internal.append("tcp://*:%s" % (next_port))
    input_ports_external.append("tcp://%s:%s" % (_hosts[i], next_port))
    next_port += 1

  #if _debug:
  #  print "internal input ports: %s" % input_ports_internal
  #  print "external input ports: %s" % input_ports_external

  # Keep track of input port connection counts ...
  input_connection_counts = [{} for name in _names]
  for output_minnow, output_port, send_pattern, input_port, input_minnows in _connections:
    for input_minnow in input_minnows:
      if input_port not in input_connection_counts[input_minnow]:
        input_connection_counts[input_minnow][input_port] = 0
      input_connection_counts[input_minnow][input_port] += 1

  #if _debug:
  #  print "connection counts: %s" % input_connection_counts

  # Keep track of output port connections ...
  output_connections = [{} for name in _names]
  for output_minnow, output_port, send_pattern, input_port, input_minnows in _connections:
    if output_port not in output_connections[output_minnow]:
      output_connections[output_minnow][output_port] = []
    output_connections[output_minnow][output_port].append((send_pattern, input_port, [input_ports_external[input_minnow] for input_minnow in input_minnows]))

  #if _debug:
  #  print "output connections: %s" % output_connections

  # Create minnows ...
  context = zmq.Context()

  processes = []
  try:
    for i in range(len(_names)):
      name = _names[i]
      rank = i

      # Create a specification for setting-up the minnow ...
      arguments = []
      arguments += _arguments[i]
#      if _debug:
#        arguments += ["--phish-debug"]
#      arguments += ["--phish-backend", "zmq"]
      arguments += ["--phish-name", name]
      arguments += ["--phish-rank", str(rank)]
      arguments += ["--phish-control-port", control_ports_internal[i]]
      arguments += ["--phish-input-port", input_ports_internal[i]]
      for port, count in input_connection_counts[i].items():
        arguments += ["--phish-input-connections", "%s+%s" % (port, count)]
      for output_port, connections in output_connections[i].items():
        for send_pattern, input_port, minnows in connections:
          arguments += ["--phish-output-connection", "%s+%s+%s+%s" % (output_port, send_pattern, input_port, "+".join(minnows))]

      if _debug:
        sys.stderr.write("%s\n" % " ".join(arguments))

      host = _hosts[i]
      if host == "localhost" or host == "127.0.0.1":
        processes.append(local_process(name, rank, arguments))
      else:
        processes.append(ssh_process(name, rank, arguments, host))

    # Tell each minnow to start processing ...
    for control_port in control_ports_external:
      socket = context.socket(zmq.REQ)
      socket.connect(control_port)
      socket.send("start")
      if socket.recv() != "ok":
        raise Exception("Minnow failed to start.")

    # Wait for processes to terminate ...
    for process in processes:
      process.wait()

  except:
    traceback.print_exc()
    for process in processes:
      process.kill()
