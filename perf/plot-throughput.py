import csv
import matplotlib.pyplot
import optparse
import os
import socket

matplotlib.pyplot.rcParams.update({"legend.fontsize" : 10})

parser = optparse.OptionParser()
parser.add_option("--title", default=socket.gethostname(), help="Figure title.  Default: %default.")
(options, arguments) = parser.parse_args()

def plot_throughput(path, label, color):
  if not os.path.exists(path):
    return
  reader = csv.reader(open(path, "r"))
  rows = [row for row in reader]
  columns = zip(*rows)
  matplotlib.pyplot.plot(columns[1][1:], columns[4][1:], label=label, color=color)
  matplotlib.pyplot.xlabel("Message Size (bytes)")
  matplotlib.pyplot.ylabel("Throughput (Mb/S)")

matplotlib.pyplot.figure(1)
matplotlib.pyplot.title(options.title)
plot_throughput("cpp-zmq-throughput-tcp.csv", "C++ / ZMQ", "orange")
plot_throughput("cpp-mpi-throughput-fastest.csv", "C++ / ZMQ", "green")
#plot_throughput("py-zmq-throughput-tcp.csv", "Python / ZMQ", "grey")
plot_throughput("cpp-phish-zmq-throughput-tcp.csv", "C++ / Phish / ZMQ", "red")
#plot_throughput("cpp-phish-mpi-throughput-tcp.csv", "C++ / Phish / MPI / TCP", "purple")
plot_throughput("cpp-phish-mpi-throughput-fastest.csv", "C++ / Phish / MPI / Fastest", "blue")
matplotlib.pyplot.legend(loc="lower right")
matplotlib.pyplot.ylim(ymin=0)

matplotlib.pyplot.show()
