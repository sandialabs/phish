// Demonstrates using the bait API to programmatically setup a phish job.
//
// Uses the graphviz backend to generate a dotfile that can be processed with
// graphviz to display a diagram of the job.  If this were a real job, you
// would link with the mpi or zmq backends to do the actual computation.

#include <phish-bait.hpp>

int main(int argc, char* argv[])
{
  phish::bait::minnows("a", std::vector<std::string>(1, "localhost"), std::vector<std::string>());

  phish::bait::minnows("b", std::vector<std::string>(3, "localhost"), std::vector<std::string>());
  phish::bait::hook("a", 0, PHISH_BAIT_DIRECT, 0, "b");

  phish::bait::minnows("c", std::vector<std::string>(1, "localhost"), std::vector<std::string>());
  phish::bait::hook("b", 0, PHISH_BAIT_SINGLE, 0, "c");

  phish::bait::minnows("d", std::vector<std::string>(3, "localhost"), std::vector<std::string>());
  phish::bait::hook("c", 0, PHISH_BAIT_ROUND_ROBIN, 0, "d");

  phish::bait::minnows("e", std::vector<std::string>(3, "localhost"), std::vector<std::string>());
  phish::bait::hook("d", 0, PHISH_BAIT_PAIRED, 0, "e");

  phish::bait::minnows("f", std::vector<std::string>(3, "localhost"), std::vector<std::string>());
  phish::bait::hook("e", 0, PHISH_BAIT_HASHED, 0, "f");

  phish::bait::minnows("g", std::vector<std::string>(3, "localhost"), std::vector<std::string>());
  phish::bait::hook("f", 0, PHISH_BAIT_BROADCAST, 0, "g");

  phish::bait::minnows("h", std::vector<std::string>(7, "localhost"), std::vector<std::string>());
  phish::bait::hook("h", 0, PHISH_BAIT_CHAIN, 0, "h");

  phish::bait::minnows("i", std::vector<std::string>(7, "localhost"), std::vector<std::string>());
  phish::bait::hook("i", 0, PHISH_BAIT_RING, 0, "i");

  phish::bait::start();

  return 0;
}
