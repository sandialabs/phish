import optparse
import time
import zmq

parser = optparse.OptionParser()
parser.add_option("--address", default="tcp://127.0.0.1:5555", help="Local address.  Default: %default")
parser.add_option("--size", type="int", default=1, help="Message size.  Default: %default")
parser.add_option("--count", type="int", default=100000, help="Message count.  Default: %default")
(options, arguments) = parser.parse_args()

context = zmq.Context()
socket = context.socket(zmq.REP)
socket.bind(options.address)

for i in range(options.count):
  message = socket.recv()
  if len(message) != options.size:
    raise Exception("message size mismatch")
  socket.send(message)

