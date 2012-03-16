import optparse
import sys
import time
import zmq

parser = optparse.OptionParser()
parser.add_option("--address", default="tcp://127.0.0.1:5555", help="Local address.  Default: %default")
parser.add_option("--size", type="int", default=1, help="Message size.  Default: %default")
parser.add_option("--count", type="int", default=100000, help="Message count.  Default: %default")
parser.add_option("--hwm", type="int", default=100000, help="High water mark.  Default: %default")
options, arguments = parser.parse_args()

context = zmq.Context()
socket = context.socket(zmq.REQ)
socket.setsockopt(zmq.HWM, options.hwm)
socket.connect(options.address)

start = time.time()

message = "*" * options.size
for i in range(options.count):
  socket.send(message)
  socket.recv()

elapsed = time.time() - start
latency = elapsed / (options.count * 2.0)

sys.stdout.write("%s,%s,%s,%s\n" % (elapsed, options.size, options.count, (latency * 1000000.0)))
