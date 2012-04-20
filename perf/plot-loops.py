import csv
import math
import matplotlib.pyplot
import os

matplotlib.pyplot.rcParams.update({"legend.fontsize" : 10})

def plot_loops(path, label, color):
  if not os.path.exists(path):
    return
  reader = csv.reader(open(path, "r"))
  rows = [row for row in reader]
  columns = zip(*rows)

  x = [float(value) for value in columns[5][1:]]
  y = [float(value) for value in columns[3][1:]]
  s = [math.pow(float(value) / 1000, 2) + 20 for value in columns[1][1:]]

  matplotlib.pyplot.scatter(x, y, s=s, label=label, color=color)
  matplotlib.pyplot.xlabel("Loop size (minnows)")
  matplotlib.pyplot.ylabel("Rate (messages/S)")

matplotlib.pyplot.figure(1)
plot_loops("cpp-phish-zmq-loop-tcp.csv", "C++ / Phish / ZMQ", "red")
plot_loops("cpp-phish-mpi-loop-tcp.csv", "C++ / Phish / MPI / TCP", "green")
plot_loops("cpp-phish-mpi-loop-sm.csv", "C++ / Phish / MPI / Fastest", "blue")
matplotlib.pyplot.legend(loc="upper right")
matplotlib.pyplot.ylim(ymin=0)

matplotlib.pyplot.show()
