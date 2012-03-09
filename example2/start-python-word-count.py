import optparse
import phish.school
import sys

parser = optparse.OptionParser()
parser.add_option("--count", type="int", default=10, help="Number of items to return.  Default: %default.")
parser.add_option("--file", default="-", help="Output file path, or - for stdout.  Default: %default.")
parser.add_option("--workers", type="int", default=2, help="Number of workers.  Default: %default.")
(options, arguments) = parser.parse_args()

filegen = phish.school.create_minnows("filegen", ["python", "filegen.py"] + arguments)
file2words = phish.school.create_minnows("file2words", ["python", "file2words.py"], options.workers)
count = phish.school.create_minnows("count", ["python", "count.py"], options.workers)
sort = phish.school.create_minnows("sort", ["python", "sort.py", "--count", str(options.count)])
print_messages = phish.school.create_minnows("print", ["python", "print.py", "--file", options.file])

phish.school.all_to_all(filegen, 0, phish.school.ROUND_ROBIN, 0, file2words)
phish.school.all_to_all(file2words, 0, phish.school.HASHED, 0, count)
phish.school.all_to_all(count, 0, phish.school.BROADCAST, 0, sort)
phish.school.one_to_one(sort, 0, 0, print_messages)

phish.school.start()

