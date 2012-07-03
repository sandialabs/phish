#include <phish-bait.hpp>

int main(int argc, char* argv[])
{
  std::vector<std::string> files(argv + 1, argv + argc);
  std::vector<std::string> filegen_hosts(1, "localhost");
  std::vector<std::string> filegen_arguments(1, "zmq-filegen");
  filegen_arguments.insert(filegen_arguments.end(), files.begin(), files.end());
  std::vector<int> filegen_minnows = phish::bait::add_minnows("filegen", filegen_hosts, filegen_arguments);

  std::vector<std::string> file2words_hosts(1, "localhost");
  std::vector<std::string> file2words_arguments(1, "zmq-file2words");
  std::vector<int> file2words_minnows = phish::bait::add_minnows("file2word", file2words_hosts, file2words_arguments);

  std::vector<std::string> count_hosts(1, "localhost");
  std::vector<std::string> count_arguments(1, "zmq-count");
  std::vector<int> count_minnows = phish::bait::add_minnows("count", count_hosts, count_arguments);

  std::vector<std::string> sort_hosts(1, "localhost");
  std::vector<std::string> sort_arguments(1, "zmq-sort");
  sort_arguments.insert(sort_arguments.end(), "20"); // Return the top-twenty words
  std::vector<int> sort_minnows = phish::bait::add_minnows("sort", sort_hosts, sort_arguments);

  std::vector<std::string> print_hosts(1, "localhost");
  std::vector<std::string> print_arguments(1, "zmq-print");
  std::vector<int> print_minnows = phish::bait::add_minnows("print", print_hosts, print_arguments);

  phish::bait::all_to_all(filegen_minnows, 0, PHISH_BAIT_ROUND_ROBIN, 0, file2words_minnows);
  phish::bait::all_to_all(file2words_minnows, 0, PHISH_BAIT_HASHED, 0, count_minnows);
  phish::bait::all_to_all(count_minnows, 0, PHISH_BAIT_BROADCAST, 0, sort_minnows);
  phish::bait::one_to_one(sort_minnows, 0, 0, print_minnows);

  phish::bait::start();

  return 0;
}
