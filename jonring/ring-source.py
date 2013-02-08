#!/usr/local/bin/python

import optparse
import phish
import sys
import time

sys.argv = phish.init(sys.argv)
my_id = phish.query("idlocal", 0, 0)

parser = optparse.OptionParser()
parser.add_option("--receiver", type="int", default=0, help="Message recipient.  Default: %default.")
parser.add_option("--message-count", "-n", type="int", default=100000, help="Message count.  Default: %default.")
parser.add_option("--message-size", "-m", type="int", default=80, help="Message size.  Default: %default.")
options, arguments = parser.parse_args()

message = "*" * options.message_size
tick_count = 0

def tick_message(field_count):
  #print "source", my_id, "tick"
  global tick_count
  if tick_count < options.message_count:
    phish.pack_string(message)
    phish.send_direct(0, options.receiver)
  elif tick_count == options.message_count:
    phish.pack_string("stop")
    phish.send_direct(0, options.receiver)
    phish.close(0)
  tick_count += 1

phish.input(1, tick_message, None, True)
phish.output(0)
phish.check()
phish.loop()
phish.exit()

