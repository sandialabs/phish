import optparse
import school
import sys

parser = optparse.OptionParser()
parser.add_option("--count", type="int", default=10000000, help="Number of messages.  Default: %default.")
parser.add_option("--size", default="0/5000/500", help="Number of bytes in each message <begin/end/step>.  Default: %default.")
(options, arguments) = parser.parse_args()

size_begin, size_end, size_step = options.size.split("/")

sys.stderr.write("Testing C++ / Phish / ZMQ throughput ...\n")
sys.stderr.flush()
sys.stdout.write("cpp-phish-zmq elapsed [s],cpp-phish-zmq message size [B],cpp-phish-zmq message count,cpp-phish-zmq rate [msg/s],cpp-phish-zmq throughput [Mb/s]\n")
sys.stdout.flush()
for size in range(int(size_begin), int(size_end), int(size_step)):
  sys.stderr.write("message size: %s\n" % (size))
  sys.stderr.flush()
  a = school.add_minnows("a", ["localhost"], ["${CMAKE_CURRENT_BINARY_DIR}/phish-zmq-throughput-a", str(size), str(options.count)])
  b = school.add_minnows("b", ["localhost"], ["${CMAKE_CURRENT_BINARY_DIR}/phish-zmq-throughput-b", str(size), str(options.count)])
  school.one_to_one(b, 0, 0, a)
  school.start()
  school.reset()

