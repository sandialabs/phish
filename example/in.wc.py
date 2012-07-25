# word count from files
# provide list of files or dirs as -v files command-line arg

minnow filegen filegen.py ${files}
minnow file2words file2words.py
minnow count count.py
minnow sort sort.py 10
minnow print print.py

hook filegen roundrobin file2words
hook file2words hashed count
hook count single sort
hook sort single print

school filegen 1
school file2words 4
school count 3
school sort 1
school print 1
