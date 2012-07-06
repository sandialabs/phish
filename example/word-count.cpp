#include <phish-bait.hpp>

int main(int argc, char* argv[])
{
  std::vector<std::string> files(argv + 1, argv + argc);
  std::vector<std::string> filegen_hosts(1, "localhost");
  std::vector<std::string> filegen_arguments(1, "filegen-zmq");
  filegen_arguments.insert(filegen_arguments.end(), files.begin(), files.end());
  phish::bait::school("filegen", filegen_hosts, filegen_arguments);

  std::vector<std::string> file2words_hosts(2, "localhost");
  std::vector<std::string> file2words_arguments(1, "file2words-zmq");
  phish::bait::school("file2words", file2words_hosts, file2words_arguments);

  std::vector<std::string> count_hosts(2, "localhost");
  std::vector<std::string> count_arguments(1, "count-zmq");
  phish::bait::school("count", count_hosts, count_arguments);

  std::vector<std::string> sort_hosts(1, "localhost");
  std::vector<std::string> sort_arguments(1, "sort-zmq");
  sort_arguments.insert(sort_arguments.end(), "20"); // Return the top-twenty words
  phish::bait::school("sort", sort_hosts, sort_arguments);

  std::vector<std::string> print_hosts(1, "localhost");
  std::vector<std::string> print_arguments(1, "print-zmq");
  phish::bait::school("print", print_hosts, print_arguments);

  phish::bait::hook("filegen", 0, PHISH_BAIT_ROUND_ROBIN, 0, "file2words");
  phish::bait::hook("file2words", 0, PHISH_BAIT_HASHED, 0, "count");
  phish::bait::hook("count", 0, PHISH_BAIT_SINGLE, 0, "sort");
  phish::bait::hook("sort", 0, PHISH_BAIT_SINGLE, 0, "print");

  phish::bait::start();

  return 0;
}
