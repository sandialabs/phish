import optparse
import re
import sys

parser = optparse.OptionParser()
parser.add_option("--graphviz", default=False, action="store_true", help="Use the graphviz backend.")
parser.add_option("--mpi", default=False, action="store_true", help="Use the MPI backend.")
parser.add_option("--mpi-config", default=False, action="store_true", help="Use the MPI configfile backend.")
parser.add_option("--variable", "-v", action="append", nargs=2, default=[], help="Verbose output.  Default: %default.")
parser.add_option("--verbose", default=False, action="store_true", help="Verbose output.  Default: %default.")
parser.add_option("--zmq", default=False, action="store_true", help="Use the ZMQ backend.")
parser.add_option("--suffix", default="", help="Add a suffix to minnow command names.")
options, arguments = parser.parse_args()

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

  # Split the line into a command ...
  arguments = line.split()
  command = arguments[0]
  arguments = arguments[1:]

  # Do variable substitution on arguments ...
  expanded = []
  for argument in arguments:
    match = re.match("\${?([^}]*)}?", argument)
    if match:
      if match.group(1) not in variables:
        raise Exception("Unknown variable '%s'" % match.group(1))
      expanded += variables[match.group(1)]
    else:
      expanded.append(argument)
  arguments = expanded

  # Currently unused by the zmq backend
  if command == "set":
    pass

  elif command == "variable":
    key = arguments[0]
    values = arguments[1:]
    if key not in variables:
      variables[key] = values

  elif command == "minnow":
    id = arguments[0]
    arguments = arguments[1:]
    schools[id] = {"arguments" : arguments, "count" : 1, "host" : "localhost"}

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

# Add suffixes to individual school arguments ...
for school in schools.values():
  school["arguments"][0] = school["arguments"][0] + options.suffix

if options.verbose:
  for school in schools.values():
    sys.stderr.write(" ".join(school["arguments"]) + "\n")
    sys.stderr.flush()

# Pass the parsed data to the bait backend ...
import bait

if options.graphviz + options.mpi + options.mpi_config + options.zmq != 1:
  raise Exception("You must specify a single backend using --graphviz, --mpi, --mpi-config, or --zmq.")

if options.graphviz:
  bait.backend("graphviz")
if options.mpi:
  bait.backend("mpi")
if options.mpi_config:
  bait.backend("mpi-config")
if options.zmq:
  bait.backend("zmq")

for id, school in schools.items():
  bait.school(id, [school["host"]] * school["count"], school["arguments"])

for output_id, output_port, style, input_port, input_id in hooks:
  bait.hook(output_id, output_port, style, input_port, input_id)

bait.start()
