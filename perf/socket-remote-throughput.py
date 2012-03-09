import optparse
import os
import socket

parser = optparse.OptionParser()
parser.add_option("--size", type="int", default=1, help="Message size.  Default: %default")
parser.add_option("--count", type="int", default=1000000, help="Message count.  Default: %default")
options, arguments = parser.parse_args()

output = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
output.connect(("127.0.0.1", 5555))

message = "*" * options.size
for i in range(options.count):
  output.send(message)
