import optparse
import pprint
import re
import school
import sys

parser = optparse.OptionParser()
parser.add_option("--variable", "-v", action="append", nargs=2, default=[], help="Verbose output.  Default: %default.")
parser.add_option("--verbose", default=False, action="store_true", help="Verbose output.  Default: %default.")
options, arguments = parser.parse_args()

variables = dict([(key, [value]) for key, value in options.variable])
minnows = {}
hooks = []

pprint.pprint(variables)

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
      expanded += variables[match.group(1)]
    else:
      expanded.append(argument)
  arguments = expanded

  if options.verbose:
    sys.stderr.write("%s %s %s\n" % (line_number, command, " ".join(arguments)))

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
    minnows[id] = {"arguments" : arguments, "count" : 1, "host" : "localhost"}

  elif command == "hook":
    sender = arguments[0]
    type = arguments[1]
    receiver = arguments[2]
    hooks.append((sender, type, receiver))

  elif command == "school":
    id = arguments[0]
    count = arguments[1]
    keywords = arguments[2:]
    minnows[id]["count"] = int(count)
    for key, value in keywords:
      if key == "host":
        minnows[id]["host"] = value
      elif key == "invoke":
        minnows[id]["arguments"] = [value] + minnows[id]["arguments"]

  else:
    raise Exception("Unknown command '%s' on line %s: %s" % (command, line_number, line))

for id, minnow in minnows.items():
  minnows[id] = school.add_minnows(id, [minnow["host"]] * minnow["count"], minnow["arguments"])

school.start()
