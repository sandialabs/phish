import optparse
import phish
import time

def message_callback(message):
  global count
  count += 1
  if count < options.count:
    phish.send(ping)
  else:
    phish.close_port(0)

def closed_callback():
  print "Elapsed time for %s pingpong of %s bytes = %s secs" % (count, len(ping), time.clock() - start_time)
  phish.loop_complete()

phish.init()
phish.enable_input_port(0, message_callback, closed_callback)
phish.enable_output_port(0)
phish.check

parser = optparse.OptionParser()
parser.add_option("--count", type="int", default=1000, help="Number of pings.  Default: %default.")
parser.add_option("--size", type="int", default=0, help="Number of bytes in each ping.  Default: %default.")
(options, arguments) = parser.parse_args()

count = 0
ping = "*" * options.size
start_time = time.clock()

phish.send(ping)
phish.loop()

