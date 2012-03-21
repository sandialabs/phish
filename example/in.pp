# ping-pong test between 2 processes

set memory 1025

minnow 1 ping 100000 10000
minnow 2 pong

connect 1 single 2
connect 2 single 1

layout 1 1
layout 2 1
