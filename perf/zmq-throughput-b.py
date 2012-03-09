import optparse
import zmq

parser = optparse.OptionParser()
parser.add_option("--address", default="tcp://127.0.0.1:5555", help="Local address.  Default: %default")
parser.add_option("--size", type="int", default=1, help="Message size.  Default: %default")
parser.add_option("--count", type="int", default=100000, help="Message count.  Default: %default")
parser.add_option("--hwm", type="int", default=1000000, help="High water mark.  Default: %default")
options, arguments = parser.parse_args()

context = zmq.Context()
output = context.socket(zmq.PUSH)
output.setsockopt(zmq.HWM, options.hwm)
output.connect(options.address)

message = "*" * options.size
for i in range(options.count):
  output.send(message)
