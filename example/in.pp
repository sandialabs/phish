# ping-pong test between 2 processes

set memory 1025

variable n 1000000
variable m 0

minnow 1 ping $n $m
minnow 2 pong

hook 1 single 2
hook 2 single 1

school 1 1
school 2 1

