import optparse
import school
import sys

parser = optparse.OptionParser()
parser.add_option("--count", type="int", default=20, help="Number of items to return.  Default: %default.")
parser.add_option("--file2word-workers", type="int", default=1, help="Number of workers.  Default: %default.")
parser.add_option("--count-workers", type="int", default=1, help="Number of workers.  Default: %default.")
(options, arguments) = parser.parse_args()

filegen = school.add_minnows("filegen", ["localhost"], ["zmq-filegen"] + arguments)
file2words = school.add_minnows("file2words", ["localhost"] * options.file2word_workers, ["zmq-file2words"])
count = school.add_minnows("count", ["localhost"] * options.count_workers, ["zmq-count"])
sort = school.add_minnows("sort", ["localhost"], ["zmq-sort", str(options.count)])
print_messages = school.add_minnows("print", ["localhost"], ["zmq-print"])

school.all_to_all(filegen, 0, school.ROUND_ROBIN, 0, file2words)
school.all_to_all(file2words, 0, school.HASHED, 0, count)
school.all_to_all(count, 0, school.BROADCAST, 0, sort)
school.one_to_one(sort, 0, 0, print_messages)

school.start()

