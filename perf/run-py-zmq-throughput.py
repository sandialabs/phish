import optparse
import perf
import subprocess
import sys

parser = optparse.OptionParser()
parser.add_option("--count", default="10000000", help="Number of messages.  Default: %default.")
parser.add_option("--size", default="0/4608/512", help="Number of bytes in each message <begin/end/step>.  Default: %default.")
parser.add_option("--hwm", default="100000", help="High water mark.  Default: %default.")
(options, arguments) = parser.parse_args()

size_begin, size_end, size_step = options.size.split("/")
hosts = perf.hosts()

sys.stderr.write("Testing Python / ZMQ throughput on %s and %s ...\n" % (hosts[0], hosts[1]))
sys.stderr.flush()
sys.stdout.write("py-zmq elapsed [s],py-zmq message size [B],py-zmq message count,py-zmq rate [msg/s],py-zmq throughput [Mb/s]\n")
sys.stdout.flush()
for size in range(int(size_begin), int(size_end), int(size_step)):
  sys.stderr.write("message size: %s\n" % (size))
  sys.stderr.flush()
  arguments = perf.arguments(
    ["python", "${CMAKE_CURRENT_SOURCE_DIR}/zmq-throughput-a.py", "--address", "tcp://*:5555", "--count", str(options.count), "--size", str(size)],
    ["python", "${CMAKE_CURRENT_SOURCE_DIR}/zmq-throughput-b.py", "--address", "tcp://%s:5555" % hosts[0], "--count", str(options.count), "--size", str(size), "--hwm", str(options.hwm)]
    )
  a = subprocess.Popen(arguments[0])
  b = subprocess.Popen(arguments[1])
  a.wait()
  b.wait()

