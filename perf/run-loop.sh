#!/bin/sh

python run-cpp-phish-zmq-loop.py --processes 2/256 > cpp-phish-zmq-loop-tcp.csv
python run-cpp-phish-mpi-loop.py --processes 2/256 > cpp-phish-mpi-loop-fastest.csv
##python run-cpp-phish-mpi-loop.py --mpi-extra="--mca btl self,tcp" > cpp-phish-mpi-loop-tcp.csv

#python run-cpp-phish-zmq-hashed.py > cpp-phish-zmq-hashed-tcp.csv
#python run-cpp-phish-mpi-hashed.py > cpp-phish-mpi-hashed-fastest.csv
##python run-cpp-phish-mpi-hashed.py --mpi-extra="--mca btl self,tcp" > cpp-phish-mpi-hashed-tcp.csv

