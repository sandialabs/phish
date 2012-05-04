#!/bin/sh

python run-cpp-phish-zmq-hashed.py --processes 2/256 > cpp-phish-zmq-hashed-tcp.csv
python run-cpp-phish-mpi-hashed.py --processes 2/256 > cpp-phish-mpi-hashed-fastest.csv
##python run-cpp-phish-mpi-hashed.py --mpi-extra="--mca btl self,tcp" > cpp-phish-mpi-hashed-tcp.csv

