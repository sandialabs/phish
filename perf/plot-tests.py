import csv
import matplotlib.pyplot

def plot_latency(path, label):
  reader = csv.reader(open(path, "r"))
  rows = [row for row in reader]
  columns = zip(*rows)
  matplotlib.pyplot.plot(columns[1][1:], columns[3][1:], label=label)
  matplotlib.pyplot.xlabel("Message Size (bytes)")
  matplotlib.pyplot.ylabel("Latency ($\mu\mathrm{S}$)")

def plot_throughput(path, label):
  reader = csv.reader(open(path, "r"))
  rows = [row for row in reader]
  columns = zip(*rows)
  matplotlib.pyplot.plot(columns[1][1:], columns[4][1:], label=label)
  matplotlib.pyplot.xlabel("Message Size (bytes)")
  matplotlib.pyplot.ylabel("Throughput (Mb/S)")

def plot_rate(path, label):
  reader = csv.reader(open(path, "r"))
  rows = [row for row in reader]
  columns = zip(*rows)
  matplotlib.pyplot.plot(columns[1][1:], columns[3][1:], label=label)
  matplotlib.pyplot.xlabel("Message Size (bytes)")
  matplotlib.pyplot.ylabel("Rate (messages/S)")

matplotlib.pyplot.figure(1)
plot_latency("cpp-zmq-latency-tcp.csv", "C++ / ZMQ")
plot_latency("py-zmq-latency-tcp.csv", "Python / ZMQ")
plot_latency("cpp-phish-zmq-latency-tcp.csv", "C++ / Phish / ZMQ")
plot_latency("cpp-phish-mpi-latency-sm.csv", "C++ / Phish / MPI / SM")
plot_latency("cpp-phish-mpi-latency-tcp.csv", "C++ / Phish / MPI / TCP")
matplotlib.pyplot.legend(loc="lower right")
matplotlib.pyplot.ylim(ymin=0)

matplotlib.pyplot.figure(2)
plot_throughput("cpp-zmq-throughput-tcp.csv", "C++ / ZMQ")
plot_throughput("py-zmq-throughput-tcp.csv", "Python / ZMQ")
plot_throughput("cpp-phish-zmq-throughput-tcp.csv", "C++ / Phish / ZMQ")
plot_throughput("cpp-phish-mpi-throughput-tcp.csv", "C++ / Phish / MPI / TCP")
matplotlib.pyplot.legend(loc="lower right")
matplotlib.pyplot.ylim(ymin=0)

matplotlib.pyplot.figure(3)
plot_rate("cpp-zmq-throughput-tcp.csv", "C++ / ZMQ")
plot_rate("py-zmq-throughput-tcp.csv", "Python / ZMQ")
plot_rate("cpp-phish-zmq-throughput-tcp.csv", "C++ / Phish / ZMQ")
plot_rate("cpp-phish-mpi-throughput-tcp.csv", "C++ / Phish / MPI / TCP")
matplotlib.pyplot.legend(loc="upper right")
matplotlib.pyplot.ylim(ymin=0)

matplotlib.pyplot.show()
