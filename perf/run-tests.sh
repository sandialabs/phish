python run-cpp-zmq-latency.py > cpp-zmq-latency-tcp.csv
python run-py-zmq-latency.py > py-zmq-latency-tcp.csv
python run-cpp-phish-zmq-latency.py > cpp-phish-zmq-latency-tcp.csv
python run-cpp-phish-mpi-latency.py > cpp-phish-mpi-latency-sm.csv
python run-cpp-phish-mpi-latency.py --mpi-extra="--mca btl ^sm" > cpp-phish-mpi-latency-tcp.csv

python run-cpp-zmq-throughput.py > cpp-zmq-throughput-tcp.csv
python run-py-zmq-throughput.py > py-zmq-throughput-tcp.csv
python run-cpp-phish-zmq-throughput.py > cpp-phish-zmq-throughput-tcp.csv
#python run-cpp-phish-mpi-throughput.py > cpp-phish-mpi-throughput-sm.csv
#python run-cpp-phish-mpi-throughput.py --mpi-extra="--mca btl ^sm" > cpp-phish-mpi-throughput-tcp.csv

#python run-cpp-zmq-latency.py --address ipc://foo > cpp-zmq-latency-ipc.csv
#python run-py-zmq-latency.py --address ipc://foo > py-zmq-latency-ipc.csv
#python run-cpp-zmq-throughput.py --address ipc://foo > cpp-zmq-throughput-ipc.csv
#python run-py-zmq-throughput.py --address ipc://foo > py-zmq-throughput-ipc.csv
#python run-cpp-phish-zmq-latency.py --address ipc://foo > cpp-phish-zmq-latency-ipc.csv
#python run-cpp-phish-zmq-throughput.py --address ipc://foo > cpp-phish-zmq-throughput-ipc.csv
