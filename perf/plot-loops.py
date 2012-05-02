import csv
import math
import matplotlib.pyplot
import optparse
import os
import socket

matplotlib.pyplot.rcParams.update({"legend.fontsize" : 10})

parser = optparse.OptionParser()
parser.add_option("--title", default=socket.gethostname(), help="Figure title.  Default: %default.")
(options, arguments) = parser.parse_args()

def plot_loops(path, label, color):
  if not os.path.exists(path):
    return
  reader = csv.reader(open(path, "r"))
  rows = [row for row in reader]
  columns = zip(*rows)

  message_size = columns[1][1:]
  loop_size = columns[5][1:]
  rate = columns[3][1:]

  for size in set(message_size):
    x = [float(loop_size[i]) for i in range(len(loop_size)) if message_size[i] == size]
    y = [float(rate[i]) for i in range(len(rate)) if message_size[i] == size]
    matplotlib.pyplot.plot(x, y, label="%s byte %s" % (size, label), color=color, linewidth=float(size) / 1024 + 1)

  matplotlib.pyplot.xlabel("Loop size (minnows)")
  matplotlib.pyplot.ylabel("Rate (messages/S)")

matplotlib.pyplot.figure(1)
matplotlib.pyplot.title(options.title)
plot_loops("cpp-phish-zmq-loop-tcp.csv", "C++ / Phish / ZMQ", "red")
plot_loops("cpp-phish-mpi-loop-tcp.csv", "C++ / Phish / MPI / TCP", "green")
plot_loops("cpp-phish-mpi-loop-sm.csv", "C++ / Phish / MPI / Fastest", "blue")
matplotlib.pyplot.legend(loc="upper right")
#matplotlib.pyplot.yscale("log")

matplotlib.pyplot.show()
