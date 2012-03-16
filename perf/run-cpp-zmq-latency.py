import optparse
import subprocess
import sys

parser = optparse.OptionParser()
parser.add_option("--address", default="tcp://127.0.0.1:5555", help="Network address.  Default: %default.")
parser.add_option("--count", type="int", default=100000, help="Number of messages.  Default: %default.")
parser.add_option("--size", default="0/5000/500", help="Number of bytes in each message <begin/end/step>.  Default: %default.")
(options, arguments) = parser.parse_args()

size_begin, size_end, size_step = options.size.split("/")

sys.stderr.write("Testing C++ / ZMQ latency ...\n")
sys.stderr.flush()
sys.stdout.write("cpp-zmq elapsed [s],cpp-zmq message size [B],cpp-zmq message count,cpp-zmq latency [us]\n")
sys.stdout.flush()

for size in range(int(size_begin), int(size_end), int(size_step)):
  sys.stderr.write("message size: %s\n" % (size))
  sys.stderr.flush()
  a = subprocess.Popen(["${CMAKE_CURRENT_BINARY_DIR}/zmq-latency-a", options.address, str(size), str(options.count)])
  b = subprocess.Popen(["${CMAKE_CURRENT_BINARY_DIR}/zmq-latency-b", options.address, str(size), str(options.count)])
  a.wait()
  b.wait()

