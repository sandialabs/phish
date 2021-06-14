import phish.bait.zmq

import optparse
import sys

parser = optparse.OptionParser()
parser.add_option("--count", type="int", default=20, help="Number of items to return.  Default: %default.")
parser.add_option("--file2word-workers", type="int", default=2, help="Number of workers.  Default: %default.")
parser.add_option("--count-workers", type="int", default=2, help="Number of workers.  Default: %default.")
(options, arguments) = parser.parse_args()

phish.bait.school("filegen", ["localhost"], ["filegen-zmq"] + arguments)
phish.bait.school("file2words", ["localhost"] * options.file2word_workers, ["file2words-zmq"])
phish.bait.school("count", ["localhost"] * options.count_workers, ["count-zmq"])
phish.bait.school("sort", ["localhost"], ["sort-zmq", str(options.count)])
phish.bait.school("print", ["localhost"], ["print-zmq"])

phish.bait.hook("filegen", 0, phish.bait.ROUND_ROBIN, 0, "file2words")
phish.bait.hook("file2words", 0, phish.bait.HASHED, 0, "count")
phish.bait.hook("count", 0, phish.bait.SINGLE, 0, "sort")
phish.bait.hook("sort", 0, phish.bait.SINGLE, 0, "print")

phish.bait.start()

