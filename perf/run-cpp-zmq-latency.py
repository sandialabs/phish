import optparse
import perf
import subprocess
import sys

parser = optparse.OptionParser()
parser.add_option("--count", type="int", default=100000, help="Number of messages.  Default: %default.")
parser.add_option("--size", default="0/5000/500", help="Number of bytes in each message <begin/end/step>.  Default: %default.")
(options, arguments) = parser.parse_args()

size_begin, size_end, size_step = options.size.split("/")
hosts = perf.hosts()

sys.stderr.write("Testing C++ / ZMQ latency on %s and %s ...\n" % (hosts[0], hosts[1]))
sys.stderr.flush()
sys.stdout.write("cpp-zmq elapsed [s],cpp-zmq message size [B],cpp-zmq message count,cpp-zmq latency [us]\n")
sys.stdout.flush()

for size in range(int(size_begin), int(size_end), int(size_step)):
  sys.stderr.write("message size: %s\n" % (size))
  sys.stderr.flush()
  arguments = perf.arguments(
    ["${CMAKE_CURRENT_BINARY_DIR}/zmq-latency-a", "tcp://%s:5555" % hosts[1], str(size), str(options.count)],
    ["${CMAKE_CURRENT_BINARY_DIR}/zmq-latency-b", "tcp://*:5555", str(size), str(options.count)]
    )
  #sys.stderr.write("%s\n" % " ".join(arguments[0]))
  #sys.stderr.write("%s\n" % " ".join(arguments[1]))
  #sys.stderr.flush()
  a = subprocess.Popen(arguments[0])
  b = subprocess.Popen(arguments[1])
  a.wait()
  b.wait()

