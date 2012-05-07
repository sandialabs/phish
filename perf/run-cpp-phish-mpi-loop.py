import optparse
import perf
import subprocess
import sys

parser = optparse.OptionParser()
parser.add_option("--count", type="int", default=10000000, help="Number of messages.  Default: %default.")
parser.add_option("--size", default="0/8192/4096", help="Number of bytes in each message <begin/end/step>.  Default: %default.")
parser.add_option("--mpi-extra", default="", help="Extra options to pass to mpiexec.  Default: %default.")
parser.add_option("--processes", default="2/256", help="Number of processes in the loop <begin/end/step>.  Default: %default.")
(options, arguments) = parser.parse_args()

size_begin, size_end, size_step = options.size.split("/")
process_begin, process_end = options.processes.split("/")

sys.stderr.write("Testing C++ / Phish / MPI loops ...\n")
sys.stderr.flush()
sys.stdout.write("elapsed [s],message size [B],message count,rate [msg/s],throughput [Mb/s],process count,host\n")
sys.stdout.flush()
for size in range(int(size_begin), int(size_end), int(size_step)):
  sys.stderr.write("message size: %s\n" % (size))
  sys.stderr.flush()
  process_count = int(process_begin)
  while True:
    if process_count >= int(process_end):
      break
    hosts = perf.hosts(process_count)
    sys.stderr.write("  process count: %s on %s\n" % (process_count, hosts))
    sys.stderr.flush()

    bait_input = """
set memory 100
set safe 10000

minnow 1 phish-mpi-loop phish-mpi-loop %s %s

connect 1 ring 1

layout 1 %s
""" % (options.count, size, process_count)

    bait = subprocess.Popen(["python", "${CMAKE_SOURCE_DIR}/bait/bait.py"], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    bait.communicate(input = bait_input)

    subprocess.check_call(["mpiexec"] + options.mpi_extra.split() + open("outfile", "r").read().split())

    process_count *= 2
