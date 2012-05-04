import csv
import math
import matplotlib.pyplot
import optparse
import os
import socket

matplotlib.pyplot.rcParams.update({"legend.fontsize" : 10})

parser = optparse.OptionParser()
parser.add_option("--title", default="Hashed Test on %s" % socket.gethostname(), help="Figure title.  Default: %default.")
(options, arguments) = parser.parse_args()

message_count = 0

def plot_hashed(path, label, color):
  if not os.path.exists(path):
    return
  reader = csv.reader(open(path, "r"))
  rows = [row for row in reader]
  columns = zip(*rows)

  message_count = int(columns[2][1])

  message_sizes = [int(value) for value in columns[1][1:]]
  loop_sizes = [int(value) for value in columns[5][1:]]
  rates = [float(value) for value in columns[3][1:]]

  grouped = {}
  for i in range(len(message_sizes)):
    message_size = message_sizes[i]
    loop_size = loop_sizes[i]
    rate = rates[i]

    if message_size not in grouped:
      grouped[message_size] = {}
    if loop_size not in grouped[message_size]:
      grouped[message_size][loop_size] = []
    grouped[message_size][loop_size].append(rate)

  for message_size in sorted(grouped.keys()):
    x = [loop_size for loop_size in sorted(grouped[message_size].keys())]
    y = [sum(grouped[message_size][loop_size]) / len(grouped[message_size][loop_size]) for loop_size in sorted(grouped[message_size].keys())]
    matplotlib.pyplot.plot(x, y, label="%s byte %s" % (message_size, label), color=color, linewidth=message_size / 4096 + 1)

  matplotlib.pyplot.xlabel("Process count (1/2 senders, 1/2 receivers)")
  matplotlib.pyplot.ylabel("Rate (messages/S)")

matplotlib.pyplot.figure(1)
matplotlib.pyplot.title(options.title)
matplotlib.pyplot.text(0.5, 0.5, "%s" % message_count, horizontalalignment="center", verticalalignment="top")
plot_hashed("cpp-phish-zmq-hashed-tcp.csv", "C++ / Phish / ZMQ", "red")
#plot_hashed("cpp-phish-mpi-hashed-tcp.csv", "C++ / Phish / MPI / TCP", "purple")
plot_hashed("cpp-phish-mpi-hashed-fastest.csv", "C++ / Phish / MPI / Fastest", "blue")
matplotlib.pyplot.legend(loc="upper left")
#matplotlib.pyplot.yscale("log")

matplotlib.pyplot.show()
