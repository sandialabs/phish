import csv
import matplotlib.pyplot
import optparse
import os
import socket

matplotlib.pyplot.rcParams.update({"legend.fontsize" : 10})

parser = optparse.OptionParser()
parser.add_option("--title", default=socket.gethostname(), help="Figure title.  Default: %default.")
(options, arguments) = parser.parse_args()

def plot_latency(path, label):
  if not os.path.exists(path):
    return
  reader = csv.reader(open(path, "r"))
  rows = [row for row in reader]
  columns = zip(*rows)
  matplotlib.pyplot.plot(columns[1][1:], columns[3][1:], label=label)
  matplotlib.pyplot.xlabel("Message Size (bytes)")
  matplotlib.pyplot.ylabel("Latency ($\mu\mathrm{S}$)")

matplotlib.pyplot.figure(1)
matplotlib.pyplot.title(options.title)
plot_latency("cpp-zmq-latency-tcp.csv", "C++ / ZMQ")
plot_latency("py-zmq-latency-tcp.csv", "Python / ZMQ")
plot_latency("cpp-phish-zmq-latency-tcp.csv", "C++ / Phish / ZMQ")
plot_latency("cpp-phish-mpi-latency-tcp.csv", "C++ / Phish / MPI / TCP")
plot_latency("cpp-phish-mpi-latency-fastest.csv", "C++ / Phish / MPI / Fastest")
matplotlib.pyplot.legend(loc="upper left")
matplotlib.pyplot.ylim(ymin=0)


matplotlib.pyplot.show()
