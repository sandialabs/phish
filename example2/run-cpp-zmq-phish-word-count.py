import optparse
import phish.school
import sys

parser = optparse.OptionParser()
parser.add_option("--count", type="int", default=20, help="Number of items to return.  Default: %default.")
parser.add_option("--file", default="-", help="Output file path, or - for stdout.  Default: %default.")
parser.add_option("--file2word-workers", type="int", default=3, help="Number of workers.  Default: %default.")
parser.add_option("--count-workers", type="int", default=6, help="Number of workers.  Default: %default.")
(options, arguments) = parser.parse_args()

phish.school.debug(True)

filegen = phish.school.create_minnows("filegen", ["zmq-filegen"] + arguments)
file2words = phish.school.create_minnows("file2words", ["zmq-file2words"], options.file2word_workers)
count = phish.school.create_minnows("count", ["zmq-count"], options.count_workers)
sort = phish.school.create_minnows("sort", ["zmq-sort", str(options.count)])
print_messages = phish.school.create_minnows("print", ["zmq-print", "--file", options.file])

phish.school.all_to_all(filegen, 0, phish.school.ROUND_ROBIN, 0, file2words)
phish.school.all_to_all(file2words, 0, phish.school.HASHED, 0, count)
phish.school.all_to_all(count, 0, phish.school.BROADCAST, 0, sort)
phish.school.one_to_one(sort, 0, 0, print_messages)

phish.school.start()

