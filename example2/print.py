import optparse
import os
import phish
import sys

def message_callback(message):
  file.write(options.format.format(pid=os.getpid(), name=phish.name(), rank=phish.rank(), message=message))

def closed_callback():
  phish.close()

phish.init()
phish.enable_input_port(0, message_callback, closed_callback)
phish.check()

parser = optparse.OptionParser()
parser.add_option("--file", default="-", help="Output file path, or - for stdout.  Default: %default.")
parser.add_option("--format", default="{name} ({rank}) - {message}\n", help="Message format string.  Use {name} for the minnow name, {rank} for the minnow rank, {pid} for the process ID, and {message} for the message.  Default: %default.")
(options, arguments) = parser.parse_args()
file = open(options.file, "w") if options.file is not "-" else sys.stdout

phish.loop()
