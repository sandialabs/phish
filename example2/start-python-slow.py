import optparse
import phish.school
import sys

parser = optparse.OptionParser()
parser.add_option("--file", default="-", help="Output file path, or - for stdout.  Default: %default.")
parser.add_option("--rate", type="float", default="2", help="Slowdown rate, in messages-per-second.  Default: %default.")
parser.add_option("--workers", type="int", default=5, help="Number of workers.  Default: %default.")
(options, arguments) = parser.parse_args()

filegen = phish.school.create_minnows("filegen", ["python", "filegen.py"] + arguments)
file2words = phish.school.create_minnows("file2words", ["python", "file2words.py"], options.workers)
slowdown = phish.school.create_minnows("slowdown", ["python", "slowdown.py", "--rate", str(options.rate)])
print_messages = phish.school.create_minnows("print", ["python", "print.py", "--file", options.file])

phish.school.all_to_all(filegen, 0, phish.school.ROUND_ROBIN, 0, file2words)
phish.school.all_to_all(file2words, 0, phish.school.HASHED, 0, slowdown)
phish.school.all_to_all(slowdown, 0, phish.school.BROADCAST, 0, print_messages)

phish.school.start()

