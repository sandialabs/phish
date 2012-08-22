# ping-pong latency test
# 2 minnows send N messages of length M back and forth

variable N 1000000
variable M 0

minnow 1 ping $N $M
minnow 2 pong

hook 1 single 2
hook 2 single 1

school 1 1
school 2 1
