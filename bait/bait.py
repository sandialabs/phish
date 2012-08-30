# Bait.py pre-processing script

import phish.bait
import optparse,os,re,sys,string

def variable_callback(option,opt_str,value,parser):
  def floatable(string):
    try:
      float(string)
      return True
    except ValueError:
      return False

  value = []
  for arg in parser.rargs:
    # stop on --foo like options
    if arg[:2] == "--" and len(arg) > 2:
      break
    # stop on -a, but not on -3 or -3.0
    if arg[:1] == "-" and len(arg) > 1 and not floatable(arg):
      break
    value.append(arg)
  del parser.rargs[:len(value)]

  value = (value[0], value[1:])
  parser.values.variable.append(value)

# compute bounds implied by numeric str with a possible wildcard asterik
# 0 = lower bound, N-1 = upper bound
# 5 possibilities:
#   (1) I = I to I, (2) * = 0 to N-1
#   (3) I* = I to N-1, (4) *J = 0 to J, (5) I*J = I to J
# return start,stop

def bounds(str,n,param):
  star = str.find('*')
  if star < 0: start = stop = int(str)
  elif star == 0 and len(str) == 1:
    start = 0
    stop = n-1
  elif star == 0:
    start = 0
    stop = int(str[1:])
  elif star == len(str)-1:
    start = int(str[:star])
    stop = n-1
  else:
    start = int(str[:star])
    stop = int(str[star+1:])
  if start < 0 or start >= n:
    raise Exception("Invalid school bind parameter %s" % param)
  if stop < 0 or stop >= n:
    raise Exception("Invalid school bind parameter %s" % param)
  if start > stop:
    raise Exception("Invalid school bind parameter %s" % param)
  return start,stop

# generate process-to-processor bindings if requested
# either by explicit school command params or a non-zero bondorder setting

def generate_bindings():
  
  # nprocs, bindorder, pernode, numnode
  # nprocs = # of minnows
  # bindorder default = 0
  # pernode default = 1
  # numnode default = # of nodes required to run nprocs

  nprocs = 0
  for id,school in schools.items(): nprocs += school["count"]

  if "bindorder" in settings: bindorder = int(settings["bindorder"])
  else: bindorder = 0
  if bindorder < 0 or bindorder > 2:
    raise Exception("Invalid bindorder setting")

  if "pernode" in settings: pernode = int(settings["pernode"])
  else: pernode = 1
  if pernode <= 0: raise Exception("Invalid pernode setting")

  if "numnode" in settings: numnode = int(settings["numnode"])
  else:
    numnode = nprocs / pernode
    if nprocs % pernode: numnode += 1
  if numnode <= 0: raise Exception("Invalid numnode setting")

  # bindflag = 1 if explicit school settings, 0 if not
  # if any school has bind params, all of them must

  bindflag = -1
  for id,school in schools.items():
    bindparams = school["bind"]
    if bindparams and bindflag < 0: bindflag = 1
    if not bindparams and bindflag < 0: bindflag = 0
    if bindparams and bindflag == 0:
      raise Exception("All or no school commands must use bind option")
    if not bindparams and bindflag:
      raise Exception("All or no school commands must use bind option")

  # generate bindings from explicit settings on each school command
  # convert bind params to list of [node,core] for each minnow instance
  # expand each bind param with wildcards -> nclist = node,core list
  # assign count instances of minnow to leading entries in nclist
    
  if bindflag:
    for id,school in schools.items():
      bindparams = school["bind"]
      nclist = []
      for param in bindparams:
        comma = param.find(',')
        if comma < 0: raise Exception("Invalid syntax in school command")
        nodes = param[:comma]
        cores = param[comma+1:]
        nodestart,nodestop = bounds(nodes,numnode,param)
        corestart,corestop = bounds(cores,pernode,param)
      if bindorder == 0 or bindorder == 1:
        for inode in range(nodestart,nodestop+1):
          for icore in range(corestart,corestop+1):
            nclist.append([inode,icore])
      elif bindorder == 2:
        for icore in range(corestart,corestop+1):
          for inode in range(nodestart,nodestop+1):
            nclist.append([inode,icore])
      bindlist = []
      for i in range(school["count"]):
        bindlist.append(nclist[i % len(nclist)])
      school["bind"] = bindlist

  # generate bindings from bindorder setting
  # only if no explicit school settings
  # always generate bindings if backend = ZMQ
  # NOTE: ZMQ part is a bit kludgy
  #       could be handled better if change args passed to backend school()
      
  if (bindorder and not bindflag) or options.backend == "zmq":
    nclist = []
    if bindorder <= 1:
      for inode in range(numnode):
        for icore in range(pernode):
          nclist.append([inode,icore])
    elif bindorder == 2:
      for icore in range(pernode):
        for inode in range(numnode):
          nclist.append([inode,icore])
    offset = 0
    for id,school in schools.items():
      bindlist = []
      for i in range(school["count"]):
        iproc = offset + i
        bindlist.append(nclist[iproc % len(nclist)])
      school["bind"] = bindlist
      offset += school["count"]
      
# --------------------------------------------------------------------

# main program
# parse command-line options

parser = optparse.OptionParser()
parser.add_option("--backend", "-b", default="mpi",
                  help="Specify the backend to use: " +
                  "mpi, mpi-config, zmq, graphviz, or null")
parser.add_option("--path", "-p", default="", metavar="PATH1:PATH2:...",
                  help="Specify a colon-delimited list of possible" +
                  "paths to prepend to executable, if file is found")
parser.add_option("--set", "-s", action="append", nargs=2,
                  default=[], metavar="NAME VALUE",
                  help="Set a backend-specific name-value pair")
parser.add_option("--suffix", "-x", default="",
                  help="Add a suffix to all minnow names")
parser.add_option("--launch", "-l", default="",
                  help="Add a launch prefix to all minnows")
parser.add_option("--variable", "-v", action="callback",
                  callback=variable_callback, dest="variable", default=[],
                  metavar="NAME VALUE", help="Specify value of a variable")
parser.add_option("--verbose", default=False, action="store_true",
                  help="Verbose output, Default: %default")
options, arguments = parser.parse_args()

paths = options.path.split(":")
settings = dict(options.set)
variables = dict([(key, value) for key,value in options.variable])
if options.verbose: settings["verbose"] = "true"
schools = {}
hooks = []

# parse the input script

if len(arguments) == 1: script = open(arguments[0], "r")
else: script = sys.stdin

for line_number, line in enumerate(script):

  # skip empty lines and commented lines

  line = line.strip()
  if len(line) == 0: continue
  if line[:1] == "#": continue

  # split line into command and a sequence of arguments

  arguments = line.split()
  command = arguments[0]
  arguments = arguments[1:]

  # variable expansion on each argument

  expanded = []
  for argument in arguments:
    match = re.match("\${?([^}]*)}?", argument)
    if match:
      name = match.group(1)
      if name not in variables:
        raise Exception("Unknown variable '%s'" % name)
      expanded += variables[name]
    else:
      expanded.append(argument)
  arguments = expanded

  if options.verbose:
    sys.stderr.write("Bait.py script: %s\n" % (" ".join([command] + arguments)))

  # handle specific commands

  if command == "set":
    if len(arguments) != 2: raise Exception("Invalid set command")
    key = arguments[0]
    value = arguments[1]
    settings[key] = value

  elif command == "variable":
    if len(arguments) == 1: raise Exception("Invalid variable command")
    key = arguments[0]
    values = arguments[1:]
    if key not in variables:
      variables[key] = values

  elif command == "minnow":
    id = arguments[0]
    arguments = arguments[1:]
    if id in schools: raise Exception("Duplicate ID in minnow command")
    schools[id] = {"index" : len(schools), "arguments" : arguments,
                   "count" : 1, "host" : [], "bind" : []}

  elif command == "hook":
    if len(arguments) != 3: raise Exception("Invalid hook command")
    output = arguments[0].split(":")
    style = arguments[1]
    input = arguments[2].split(":")
    hooks.append((output[0],int(output[1]) if len(output) > 1 else 0,
                  style, int(input[1]) if len(input) > 1 else 0, input[0]))

  # school has bind option
    
  elif command == "school":
    if len(arguments) < 2: raise Exception("Invalid school command")
    id = arguments[0]
    count = arguments[1]
    keywords = arguments[2:]
    if id not in schools: raise Exception("Unknown minnow ID in school command")
    schools[id]["count"] = int(count)
    iarg = 0
    while iarg < len(keywords):
      if keywords[iarg] == "bind":
        if iarg+2 > len(keywords):
          raise Exception("Invalid syntax in school command")
        iargstart = iarg+1
        iarg = iargstart
        while iarg < len(keywords):
          if keywords[iarg][0] != '*' and \
                keywords[iarg][0] not in string.digits: break
          iarg += 1
        schools[id]["bind"] = keywords[iargstart:iarg]
      else: raise Exception("Invalid syntax in school command")
          
  else:
    raise Exception("Unknown command '%s' on line %s: %s" %
                    (command, line_number, line))

if len(schools) == 0: raise Exception("No minnows defined")

# generate process-to-processor bindings if requested

generate_bindings()

# add suffixes to school executables
# assumes executable is 1st arg

for school in schools.values():
  executable = school["arguments"][0]
  executable = executable + options.suffix
  school["arguments"][0] = executable

# prepend path to school executables if find it
# assumes executable is 1st arg

for school in schools.values():
  executable = school["arguments"][0]
  for path in paths:
    candidate = os.path.join(path, executable)
    if os.path.exists(candidate):
      if os.access(candidate, os.F_OK):
        executable = candidate
    else:
      if options.launch == "":
        raise Exception("Cannot locate executable '%s', you may need to specify --path or --suffix." % executable)

  school["arguments"][0] = executable

# prepend launch string to school executables

for school in schools.values():
  school["arguments"] = options.launch.split() + school["arguments"]

# optionally display school command lines

if options.verbose:
  for name, value in settings.items():
    sys.stderr.write("BAIT Setting: %s %s\n" % (name, value))

# for MPI, write bind info (if it exists) to file "rankfile"
#   this is in OpenMPI format for use with mpirun -rf rankfile
# for ZMQ, bindlist always exists, create host list for each school
# NOTE: code for this should be moved to back-end

bindlist = []
for id,school in sorted(schools.items(), key=lambda x: x[1]["index"]):
  bindlist += school["bind"]

if options.backend == "mpi" or options.backend == "mpi-config":
  if bindlist:
    fp = open("rankfile","w")
    for i,pair in enumerate(bindlist):
      print >>fp,"rank %d=+n%d slot=%d" % (i,pair[0],pair[1])
    fp.close()

if options.backend == "zmq":
  if "hostnames" not in variables:
    raise Exception("Hostnames variable is required for ZMQ backend")
  hostnames = variables["hostnames"]
  for id,school in sorted(schools.items(), key=lambda x: x[1]["index"]):
    host = []
    bindpairs = school["bind"]
    for pair in bindpairs:
      host.append(hostnames[pair[0] % len(hostnames)])
    school["host"] = host
    
if not options.backend == "zmq":
  for id,school in sorted(schools.items(), key=lambda x: x[1]["index"]):
    school["host"] = school["count"] * [""]
    
# NOTE: should this give an error if backend is not built?
#       e.g. ZMQ backend just does nothing if not built

phish.bait.backend(options.backend)

# pass parsed data to the Bait.py backend
# output the schools in minnow order

for name,value in settings.items():
  phish.bait.set(name,value)

for id,school in sorted(schools.items(), key=lambda x: x[1]["index"]):
  phish.bait.school(id,school["host"],school["bind"],school["arguments"])

for output_id,output_port,style,input_port,input_id in hooks:
  phish.bait.hook(output_id,output_port,style,input_port,input_id)

phish.bait.start()
