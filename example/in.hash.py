# hash test
# S procs send to R procs
# each source proc sends N/S messages

#set safe

variable n 10000000
variable len 0
variable s 1
variable r 1

minnow 1 source.py $n ${len}
minnow 2 reduce.py

connect 1 hashed 2
connect 2 direct 1

layout 1 $s
layout 2 $r
