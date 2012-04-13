#include <phish-school.hpp>

int main(int argc, char* argv[])
{
  std::vector<std::string> files(argv + 1, argv + argc);
  std::vector<std::string> filegen_hosts(1, "localhost");
  std::vector<std::string> filegen_arguments(1, "zmq-filegen");
  filegen_arguments.insert(filegen_arguments.end(), files.begin(), files.end());
  std::vector<int> filegen_minnows = phish::school::add_minnows("filegen", filegen_hosts, filegen_arguments);

  std::vector<std::string> print_hosts(1, "localhost");
  std::vector<std::string> print_arguments(1, "zmq-print");
  std::vector<int> print_minnows = phish::school::add_minnows("print", print_hosts, print_arguments);

  phish::school::all_to_all(filegen_minnows, 0, "broadcast", 0, print_minnows);
/*
import optparse
import phish2.school as school
import sys

parser = optparse.OptionParser()
parser.add_option("--count", type="int", default=20, help="Number of items to return.  Default: %default.")
parser.add_option("--file2word-workers", type="int", default=1, help="Number of workers.  Default: %default.")
parser.add_option("--count-workers", type="int", default=1, help="Number of workers.  Default: %default.")
(options, arguments) = parser.parse_args()

school.debug(True)

filegen = school.create_minnows("filegen", ["zmq-filegen"] + arguments)
file2words = school.create_minnows("file2words", ["zmq-file2words"], options.file2word_workers)
count = school.create_minnows("count", ["zmq-count"], options.count_workers)
sort = school.create_minnows("sort", ["zmq-sort", str(options.count)])
print_messages = school.create_minnows("print", ["zmq-print"])

school.all_to_all(filegen, 0, school.ROUND_ROBIN, 0, file2words)
school.all_to_all(file2words, 0, school.HASHED, 0, count)
school.all_to_all(count, 0, school.BROADCAST, 0, sort)
school.one_to_one(sort, 0, 0, print_messages)
*/
  phish::school::start();

  return 0;
}
