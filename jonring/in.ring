minnow source ring-source
minnow workers ring-worker

hook source:0 direct workers:0
hook workers:1 single source:1
hook workers:2 ring workers:2

school source 1
school workers 3
