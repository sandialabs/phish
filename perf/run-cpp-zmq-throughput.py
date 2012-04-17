import optparse
import perf
import subprocess
import sys

parser = optparse.OptionParser()
parser.add_option("--count", default="10000000", help="Number of messages.  Default: %default.")
parser.add_option("--size", default="0/5000/500", help="Number of bytes in each message <begin/end/step>.  Default: %default.")
parser.add_option("--hwm", default="100000", help="High water mark.  Default: %default.")
(options, arguments) = parser.parse_args()

size_begin, size_end, size_step = options.size.split("/")
hosts = perf.hosts()

sys.stderr.write("Testing C++ / ZMQ throughput on %s and %s...\n" % (hosts[0], hosts[1]))
sys.stderr.flush()
sys.stdout.write("cpp-zmq elapsed [s],cpp-zmq message size [B],cpp-zmq message count,cpp-zmq rate [msg/s],cpp-zmq throughput [Mb/s]\n")
sys.stdout.flush()
for size in range(int(size_begin), int(size_end), int(size_step)):
  sys.stderr.write("message size: %s\n" % (size))
  sys.stderr.flush()
  arguments = perf.arguments(
    ["${CMAKE_CURRENT_BINARY_DIR}/zmq-throughput-a", "tcp://*:5555", str(size), options.count],
    ["${CMAKE_CURRENT_BINARY_DIR}/zmq-throughput-b", "tcp://%s:5555" % hosts[0], str(size), options.count, options.hwm]
    )
  a = subprocess.Popen(arguments[0])
  b = subprocess.Popen(arguments[1])
  a.wait()
  b.wait()

