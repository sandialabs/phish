#!/usr/local/bin/python

import optparse
import phish
import sys

sys.argv = phish.init(sys.argv)

parser = optparse.OptionParser()
parser.add_option("--receiver", type="int", default=0, help="Message recipient.  Default: %default.")
parser.add_option("--message-count", "-n", type="int", default=1000000, help="Message count.  Default: %default.")
parser.add_option("--message-size", "-m", type="int", default=80, help="Message size.  Default: %default.")
options, arguments = parser.parse_args()

phish.output(0)
phish.check()

message = "*" * options.message_size
for n in range(options.message_count):
  phish.pack_string(message)
  phish.send_direct(0, options.receiver)

phish.pack_string("stop")
phish.send_direct(0, options.receiver)

phish.close(0)
phish.exit()
