import optparse
import subprocess
import sys

parser = optparse.OptionParser()
parser.add_option("--address", default="tcp://127.0.0.1:5555", help="Network address.  Default: %default.")
parser.add_option("--count", default="10000000", help="Number of messages.  Default: %default.")
parser.add_option("--size", default="0/5000/100", help="Number of bytes in each message <begin/end/step>.  Default: %default.")
parser.add_option("--hwm", default="1000000", help="High water mark.  Default: %default.")
(options, arguments) = parser.parse_args()

size_begin, size_end, size_step = options.size.split("/")

sys.stdout.write("cpp-zmq elapsed [s],cpp-zmq message size [B],cpp-zmq message count,cpp-zmq rate [msg/s],cpp-zmq throughput [Mb/s]\n")
sys.stdout.flush()
for size in range(int(size_begin), int(size_end), int(size_step)):
  sys.stderr.write("size: %s\n" % (size))
  sys.stderr.flush()
  a = subprocess.Popen(["${CMAKE_CURRENT_BINARY_DIR}/zmq-throughput-a", options.address, str(size), options.count])
  b = subprocess.Popen(["${CMAKE_CURRENT_BINARY_DIR}/zmq-throughput-b", options.address, str(size), options.count, options.hwm])
  a.wait()
  b.wait()

