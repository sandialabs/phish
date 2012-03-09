import optparse
import phish.school
import sys

parser = optparse.OptionParser()
parser.add_option("--count", type="int", default=100000, help="Number of messages.  Default: %default.")
parser.add_option("--size", default="0/5000/100", help="Number of bytes in each message <begin/end/step>.  Default: %default.")
(options, arguments) = parser.parse_args()

phish.school.debug(True)

size_begin, size_end, size_step = options.size.split("/")

sys.stdout.write("cpp-phish-zmq elapsed [s],cpp-phish-zmq message size [B],cpp-phish-zmq message count,cpp-phish-zmq latency [us]\n")
sys.stdout.flush()
for size in range(int(size_begin), int(size_end), int(size_step)):
  sys.stderr.write("size: %s\n" % (size))
  sys.stderr.flush()
  a = phish.school.create_minnows("a", ["${CMAKE_CURRENT_BINARY_DIR}/phish-zmq-latency-a", str(size), str(options.count)])
  b = phish.school.create_minnows("b", ["${CMAKE_CURRENT_BINARY_DIR}/phish-zmq-latency-b", str(size), str(options.count)])
  phish.school.one_to_one(a, 0, 0, b)
  phish.school.one_to_one(b, 0, 0, a)
  phish.school.start()
  phish.school.reset()

