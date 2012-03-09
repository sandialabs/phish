import optparse
import os
import socket
import time

parser = optparse.OptionParser()
parser.add_option("--size", type="int", default=1, help="Message size.  Default: %default")
parser.add_option("--count", type="int", default=1000000, help="Message count.  Default: %default")
(options, arguments) = parser.parse_args()

#try:
#  os.remove("temp")
#except:
#  pass

#input = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
input = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
input.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
input.bind(("", 5555))
input.listen(1)

connection, address = input.accept()

start = time.time()

for i in range(options.count):
  message = connection.recv(options.size)
  if len(message) != options.size:
    raise Exception("message size mismatch")

elapsed = time.time() - start
throughput = options.count / elapsed
megabits = (throughput * options.size * 8) / 1000000

print "elapsed time: %s s" % elapsed
print "message size: %s [B]" % options.size
print "message count: %s" % options.count
print "mean throughput: %s [msg/s]" % throughput
print "mean throughput: %s [Mb/s]" % megabits
