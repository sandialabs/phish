import optparse
import perf
import subprocess
import sys

parser = optparse.OptionParser()
parser.add_option("--count", default="10000000", help="Number of messages.  Default: %default.")
parser.add_option("--size", default="0/4608/512", help="Number of bytes in each message <begin/end/step>.  Default: %default.")
(options, arguments) = parser.parse_args()

size_begin, size_end, size_step = options.size.split("/")
hosts = perf.hosts()

sys.stderr.write("Testing C++ / MPI throughput on %s and %s...\n" % (hosts[0], hosts[1]))
sys.stderr.flush()
sys.stdout.write("cpp-zmq elapsed [s],cpp-zmq message size [B],cpp-zmq message count,cpp-zmq rate [msg/s],cpp-zmq throughput [Mb/s]\n")
sys.stdout.flush()
for size in range(int(size_begin), int(size_end), int(size_step)):
  sys.stderr.write("message size: %s\n" % (size))
  sys.stderr.flush()
  arguments = ["mpiexec", "-np", "2", "${CMAKE_CURRENT_BINARY_DIR}/zmq-throughput", options.count, str(size)]
  a = subprocess.Popen(arguments)
  a.wait()

