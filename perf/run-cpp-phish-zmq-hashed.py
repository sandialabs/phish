import optparse
import perf
import bait
import sys

parser = optparse.OptionParser()
parser.add_option("--count", type="int", default=10000000, help="Number of messages.  Default: %default.")
parser.add_option("--size", default="0/8192/4096", help="Number of bytes in each message <begin/end/step>.  Default: %default.")
parser.add_option("--processes", default="2/64", help="Number of processes in the loop <begin/end/step>.  Default: %default.")
(options, arguments) = parser.parse_args()

size_begin, size_end, size_step = options.size.split("/")
process_begin, process_end = options.processes.split("/")

sys.stderr.write("Testing C++ / Phish / ZMQ hashed ...\n")
sys.stderr.flush()
sys.stdout.write("cpp-phish-zmq elapsed [s],cpp-phish-zmq message size [B],cpp-phish-zmq message count,cpp-phish-zmq rate [msg/s],cpp-phish-zmq throughput [Mb/s],process count\n")
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
    source = bait.add_minnows("source", hosts[:process_count / 2], ["${CMAKE_CURRENT_BINARY_DIR}/phish-zmq-source", str(options.count), str(size)])
    map = bait.add_minnows("map", hosts[process_count / 2:], ["${CMAKE_CURRENT_BINARY_DIR}/phish-zmq-map"])
    bait.all_to_all(source, 0, bait.HASHED, 0, map)
    bait.one_to_one(map, 0, 0, source)
    bait.start()
    bait.reset()

    process_count *= 2
