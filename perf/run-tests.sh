#python run-cpp-zmq-latency.py --size "0/10500/500" > cpp-zmq-latency-tcp.csv
#python run-py-zmq-latency.py --size "0/10500/500" > py-zmq-latency-tcp.csv
#python run-cpp-phish-zmq-latency.py --size "0/10500/500" > cpp-phish-zmq-latency-tcp.csv
#python run-cpp-phish-mpi-latency.py --size "0/10500/500" --mpi-extra="" > cpp-phish-mpi-latency-sm.csv
#python run-cpp-phish-mpi-latency.py --size "0/10500/500" --mpi-extra="--mca btl self,tcp" > cpp-phish-mpi-latency-tcp.csv

#python run-cpp-zmq-throughput.py --size "0/10500/500" > cpp-zmq-throughput-tcp.csv
#python run-py-zmq-throughput.py --size "0/10500/500" > py-zmq-throughput-tcp.csv
#python run-cpp-phish-zmq-throughput.py --size "0/10500/500" > cpp-phish-zmq-throughput-tcp.csv
#python run-cpp-phish-mpi-throughput.py --size "0/10500/500" --mpi-extra="" > cpp-phish-mpi-throughput-sm.csv
#python run-cpp-phish-mpi-throughput.py --size "0/10500/500" --mpi-extra="--mca btl self,tcp" > cpp-phish-mpi-throughput-tcp.csv

python run-cpp-phish-zmq-loop.py > cpp-phish-zmq-loop-tcp.csv
python run-cpp-phish-mpi-loop.py > cpp-phish-mpi-loop-sm.csv
python run-cpp-phish-mpi-loop.py --mpi-extra="--mca btl self,tcp" > cpp-phish-mpi-loop-tcp.csv

python run-cpp-phish-zmq-hashed.py > cpp-phish-zmq-hashed-tcp.csv
python run-cpp-phish-mpi-hashed.py > cpp-phish-mpi-hashed-sm.csv
python run-cpp-phish-mpi-hashed.py --mpi-extra="--mca btl self,tcp" > cpp-phish-mpi-hashed-tcp.csv

