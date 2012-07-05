import bait.zmq

import optparse
import sys

parser = optparse.OptionParser()
parser.add_option("--count", type="int", default=20, help="Number of items to return.  Default: %default.")
parser.add_option("--file2word-workers", type="int", default=2, help="Number of workers.  Default: %default.")
parser.add_option("--count-workers", type="int", default=2, help="Number of workers.  Default: %default.")
(options, arguments) = parser.parse_args()

bait.minnows("filegen", ["localhost"], ["zmq-filegen"] + arguments)
bait.minnows("file2words", ["localhost"] * options.file2word_workers, ["zmq-file2words"])
bait.minnows("count", ["localhost"] * options.count_workers, ["zmq-count"])
bait.minnows("sort", ["localhost"], ["zmq-sort", str(options.count)])
bait.minnows("print", ["localhost"], ["zmq-print"])

bait.hook("filegen", 0, bait.ROUND_ROBIN, 0, "file2words")
bait.hook("file2words", 0, bait.HASHED, 0, "count")
bait.hook("count", 0, bait.SINGLE, 0, "sort")
bait.hook("sort", 0, bait.SINGLE, 0, "print")

bait.start()

