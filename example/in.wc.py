# word count from files
# provide list of files or dirs as -v files command-line arg

minnow filegen python filegen.py ${files}
minnow file2words python file2words.py
minnow count python count.py
minnow sort python sort.py 10
minnow print python print.py

hook filegen roundrobin file2words
hook file2words hashed count
hook count single sort
hook sort single print

school filegen 1
school file2words 4
school count 3
school sort 1
school print 1
