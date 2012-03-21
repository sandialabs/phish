import optparse
import subprocess
import sys

parser = optparse.OptionParser()
parser.add_option("--count", type="int", default=100000, help="Number of messages.  Default: %default.")
parser.add_option("--mpi-extra", default="", help="Extra options to pass to mpiexec.  Default: %default.")
parser.add_option("--size", default="0/5000/500", help="Number of bytes in each message <begin/end/step>.  Default: %default.")
(options, arguments) = parser.parse_args()

size_begin, size_end, size_step = options.size.split("/")

sys.stderr.write("Testing C++ / Phish / MPI latency ...\n")
sys.stderr.flush()
sys.stdout.write("cpp-phish-mpi elapsed [s],cpp-phish-mpi message size [B],cpp-phish-mpi message count,cpp-phish-mpi latency [us]\n")
sys.stdout.flush()
for size in range(int(size_begin), int(size_end), int(size_step)):
  sys.stderr.write("message size: %s\n" % (size))
  sys.stderr.flush()

  bait_input = """
set memory 100

minnow 1 phish-mpi-latency-a %s %s
minnow 2 phish-mpi-latency-b %s %s

connect 1 single 2
connect 2 single 1

layout 1 1
layout 2 1
""" % (size, options.count, size, options.count)

  bait = subprocess.Popen(["python", "${CMAKE_SOURCE_DIR}/bait/bait.py"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  bait.communicate(input = bait_input)

  subprocess.check_call(["mpiexec"] + options.mpi_extra.split() + open("outfile", "r").read().split())
