import csv
import math
import matplotlib.pyplot
import os

matplotlib.pyplot.rcParams.update({"legend.fontsize" : 10})

def plot_hashed(path, label, color):
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
    matplotlib.pyplot.scatter(x, y, label="%s byte %s" % (size, label), marker = "o" if size == "0" else "x", color=color)

  matplotlib.pyplot.xlabel("Sender / receiver count (minnows)")
  matplotlib.pyplot.ylabel("Rate (messages/S)")

matplotlib.pyplot.figure(1)
plot_hashed("cpp-phish-zmq-hashed-tcp.csv", "C++ / Phish / ZMQ", "red")
plot_hashed("cpp-phish-mpi-hashed-tcp.csv", "C++ / Phish / MPI / TCP", "green")
plot_hashed("cpp-phish-mpi-hashed-sm.csv", "C++ / Phish / MPI / Fastest", "blue")
matplotlib.pyplot.legend(loc="upper right")
#matplotlib.pyplot.yscale("log")

matplotlib.pyplot.show()
