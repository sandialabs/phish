# Bait.py pre-processing script

import phish.bait
import optparse
import os
import re
import sys

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
parser.add_option("--suffix", default="",
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
    key = arguments[0]
    value = arguments[1]
    settings[key] = value

  elif command == "variable":
    key = arguments[0]
    values = arguments[1:]
    if key not in variables:
      variables[key] = values

  elif command == "minnow":
    id = arguments[0]
    arguments = arguments[1:]
    if id in schools:
      raise Exception("Minnow cannot use duplicate id '%s'" % id)
    schools[id] = {"index" : len(schools), "arguments" : arguments,
                   "count" : 1, "host" : ""}

  elif command == "hook":
    output = arguments[0].split(":")
    style = arguments[1]
    input = arguments[2].split(":")
    hooks.append((output[0],int(output[1]) if len(output) > 1 else 0,
                  style, int(input[1]) if len(input) > 1 else 0, input[0]))

  elif command == "school":
    id = arguments[0]
    count = arguments[1]
    keywords = arguments[2:]
    schools[id]["count"] = int(count)
    for key,value in keywords:
      if key == "host": schools[id]["host"] = value

  else:
    raise Exception("Unknown command '%s' on line %s: %s" %
                    (command, line_number, line))

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
    if os.access(os.path.join(path, executable), os.F_OK):
      executable = os.path.join(path, executable)
  school["arguments"][0] = executable

# prepend launch string to school executables

for school in schools.values():
  school["arguments"] = options.launch.split() + school["arguments"]

# optionally display school command lines

if options.verbose:
  for name, value in settings.items():
    sys.stderr.write("BAIT Setting: %s %s\n" % (name, value))

# pass parsed data to the Bait.py backend
# output schools in minnow order

phish.bait.backend(options.backend)

for name,value in settings.items():
  phish.bait.set(name,value)

for id,school in sorted(schools.items(), key=lambda x: x[1]["index"]):
  phish.bait.school(id,[school["host"]] * school["count"],school["arguments"])

for output_id,output_port,style,input_port,input_id in hooks:
  phish.bait.hook(output_id,output_port,style,input_port,input_id)

phish.bait.start()
