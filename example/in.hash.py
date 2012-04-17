# hash test
# S procs send to M procs which send to R procs
# each source proc sends N/S messages

#set safe

variable n 100
variable len 0
variable s 1
variable m 2
variable r 2

minnow 1 source.py $n ${len}
minnow 2 map.py
minnow 3 reduce.py

connect 1 hashed 2
connect 2 hashed 3
connect 3 direct 1

layout 1 $s
layout 2 $m
layout 3 $r
