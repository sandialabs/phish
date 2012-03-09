import optparse
import sys
import time
import zmq

parser = optparse.OptionParser()
parser.add_option("--address", default="tcp://127.0.0.1:5555", help="Local address.  Default: %default")
parser.add_option("--size", type="int", default=1, help="Message size.  Default: %default")
parser.add_option("--count", type="int", default=100000, help="Message count.  Default: %default")
(options, arguments) = parser.parse_args()

context = zmq.Context()
input = context.socket(zmq.PULL)
input.bind(options.address)

# Don't start timing until we receive our first message ...
message = input.recv()
if len(message) != options.size:
  raise Exception("message size mismatch")

start = time.time()

for i in range(options.count - 1):
  message = input.recv()
  if len(message) != options.size:
    raise Exception("message size mismatch")

elapsed = time.time() - start
throughput = options.count / elapsed
megabits = (throughput * options.size * 8) / 1000000

sys.stdout.write("%s,%s,%s,%s,%s\n" % (elapsed, options.size, options.count, throughput, megabits))

