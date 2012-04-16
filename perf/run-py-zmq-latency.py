import optparse
import perf
import subprocess
import sys

parser = optparse.OptionParser()
parser.add_option("--address", default="tcp://127.0.0.1:5555", help="Network address.  Default: %default.")
parser.add_option("--count", type="int", default=100000, help="Number of messages.  Default: %default.")
parser.add_option("--size", default="0/5000/500", help="Number of bytes in each message <begin/end/step>.  Default: %default.")
(options, arguments) = parser.parse_args()

size_begin, size_end, size_step = options.size.split("/")
hosts = perf.hosts()

sys.stderr.write("Testing Python / ZMQ latency on %s and %s ...\n" % (hosts[0], hosts[1]))
sys.stderr.flush()
sys.stdout.write("py-zmq elapsed [s],py-zmq message size [B],py-zmq message count,py-zmq latency [us]\n")
sys.stdout.flush()

for size in range(int(size_begin), int(size_end), int(size_step)):
  sys.stderr.write("message size: %s\n" % (size))
  sys.stderr.flush()
  arguments = perf.arguments(
    ["python", "${CMAKE_CURRENT_SOURCE_DIR}/zmq-latency-a.py", "--address", options.address, "--count", str(options.count), "--size", str(size)],
    ["python", "${CMAKE_CURRENT_SOURCE_DIR}/zmq-latency-b.py", "--address", options.address, "--count", str(options.count), "--size", str(size)]
    )
  a = subprocess.Popen(arguments[0])
  b = subprocess.Popen(arguments[1])
  a.wait()
  b.wait()

