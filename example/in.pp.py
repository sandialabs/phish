# ping-pong test between 2 processes

set memory 1025

variable n 1000000
variable m 0

minnow 1 ping.py $n $m
minnow 2 pong.py

connect 1 single 2
connect 2 single 1

layout 1 1
layout 2 1
