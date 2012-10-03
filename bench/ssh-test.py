import optparse
import StringIO
import subprocess
import sys
import time

parser = optparse.OptionParser()
parser.add_option("--host", default=None, help="Specify the host.")
parser.add_option("--delay", default=0, type="float", help="Specify a delay between connections.  Default: %defaults")
options, arguments = parser.parse_args()

if options.host is None:
  options.host = subprocess.check_output(["scontrol", "show", "hostnames"]).split()[0]

for n in [2,4,8,16,32,64,128,256]:
  sys.stderr.write("Making %s connections to %s with a %ss delay\n" % (n, options.host, options.delay))
  processes = []
  for i in range(n):
    processes.append(subprocess.Popen(["ssh", options.host, "sleep", "1000"], stdout=subprocess.PIPE, stderr=subprocess.PIPE))
    time.sleep(options.delay)

  poll = [process.poll() for process in processes]
  for process in poll:
    sys.stderr.write("." if process is None else "E")
  sys.stderr.write("\n")

  for index, process in enumerate(processes):
    if poll[index] is None:
      process.terminate()
  time.sleep(4.0)
