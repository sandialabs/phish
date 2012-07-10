import phish.bait
import optparse
import os
import re
import sys

parser = optparse.OptionParser()
parser.add_option("--graphviz", default=False, action="store_true", help="Use the graphviz backend.")
parser.add_option("--mpi", default=False, action="store_true", help="Use the MPI backend.")
parser.add_option("--mpi-config", default=False, action="store_true", help="Use the MPI config file backend.")
parser.add_option("--path", default="", help="Specify a colon-delimited list of paths to be prepended to executable names.")
parser.add_option("--set", "-s", action="append", nargs=2, default=[], help="Set a backend-specific name-value pair.")
parser.add_option("--suffix", default="", help="Add a common suffix to minnow executables.")
parser.add_option("--variable", "-v", action="append", nargs=2, default=[], help="Specify a variable value.")
parser.add_option("--verbose", default=False, action="store_true", help="Verbose output.  Default: %default.")
parser.add_option("--zmq", default=False, action="store_true", help="Use the ZMQ backend.")
options, arguments = parser.parse_args()

paths = options.path.split(":")
settings = dict(options.set)
variables = dict([(key, [value]) for key, value in options.variable])

schools = {}
hooks = []

# Parse the input script ...
if len(arguments) == 1:
  script = open(arguments[0], "r")
else:
  script = sys.stdin

for line_number, line in enumerate(script):

  # Skip empty lines ...
  line = line.strip()
  if len(line) == 0:
    continue

  # Skip commented lines ...
  if line[:1] == "#":
    continue

  # Split the line into a command and a sequence of arguments ...
  arguments = line.split()
  command = arguments[0]
  arguments = arguments[1:]

  # Do variable expansion on each argument ...
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
    schools[id] = {"arguments" : arguments, "count" : 1, "host" : ""}

  elif command == "hook":
    output = arguments[0].split(":")
    style = arguments[1]
    input = arguments[2].split(":")
    hooks.append((output[0], int(output[1]) if len(output) > 1 else 0, style, int(input[1]) if len(input) > 1 else 0, input[0]))

  elif command == "school":
    id = arguments[0]
    count = arguments[1]
    keywords = arguments[2:]
    schools[id]["count"] = int(count)
    for key, value in keywords:
      if key == "host":
        schools[id]["host"] = value
      elif key == "invoke":
        schools[id]["arguments"] = [value] + schools[id]["arguments"]

  else:
    raise Exception("Unknown command '%s' on line %s: %s" % (command, line_number, line))

# Add suffixes to school executables ...
for school in schools.values():
  executable = school["arguments"][0]
  executable = executable + options.suffix
  school["arguments"][0] = executable

# Locate school executables ...
for school in schools.values():
  executable = school["arguments"][0]
  for path in paths:
    if os.access(os.path.join(path, executable), os.X_OK):
      executable = os.path.join(path, executable)
  school["arguments"][0] = executable

# Optionally display all of the school command lines ...
if options.verbose:
  for name, value in settings.items():
    sys.stderr.write("Launch setting: %s %s\n" % (name, value))

  for school in schools.values():
    sys.stderr.write("Launch school:  %s\n" % (" ".join(school["arguments"])))
    sys.stderr.flush()

# Pass the parsed data to the bait backend ...
if options.graphviz + options.mpi + options.mpi_config + options.zmq != 1:
  raise Exception("You must specify a single backend using --graphviz, --mpi, --mpi-config, or --zmq.")

if options.graphviz:
  phish.bait.backend("graphviz")
if options.mpi:
  phish.bait.backend("mpi")
if options.mpi_config:
  phish.bait.backend("mpi-config")
if options.zmq:
  phish.bait.backend("zmq")

for name, value in settings.items():
  phish.bait.set(name, value)

for id, school in schools.items():
  phish.bait.school(id, [school["host"]] * school["count"], school["arguments"])

for output_id, output_port, style, input_port, input_id in hooks:
  phish.bait.hook(output_id, output_port, style, input_port, input_id)

phish.bait.start()
