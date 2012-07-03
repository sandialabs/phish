import optparse
importbait 
import sys

parser = optparse.OptionParser()
parser.add_option("--count", type="int", default=20, help="Number of items to return.  Default: %default.")
parser.add_option("--file2word-workers", type="int", default=1, help="Number of workers.  Default: %default.")
parser.add_option("--count-workers", type="int", default=1, help="Number of workers.  Default: %default.")
(options, arguments) = parser.parse_args()

filegen = bait.add_minnows("filegen", ["localhost"], ["zmq-filegen"] + arguments)
file2words = bait.add_minnows("file2words", ["localhost"] * options.file2word_workers, ["zmq-file2words"])
count = bait.add_minnows("count", ["localhost"] * options.count_workers, ["zmq-count"])
sort = bait.add_minnows("sort", ["localhost"], ["zmq-sort", str(options.count)])
print_messages = bait.add_minnows("print", ["localhost"], ["zmq-print"])

bait.all_to_all(filegen, 0, bait.ROUND_ROBIN, 0, file2words)
bait.all_to_all(file2words, 0, bait.HASHED, 0, count)
bait.all_to_all(count, 0, bait.BROADCAST, 0, sort)
bait.one_to_one(sort, 0, 0, print_messages)

bait.start()

