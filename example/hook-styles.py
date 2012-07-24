# Demonstrates using the Python bindings to the bait API to programmatically
# setup a Phish job.
#
# Uses the graphviz backend to generate a dotfile that can be processed with
# graphviz to display a diagram of the job.  If this were a real job, you would
# replace "import bait.graphviz" with "import bait.mpi" or "import bait.zmq" to
# do the actual computation.

import phish.bait.graphviz

phish.bait.school("a", ["localhost"] * 1, []);

phish.bait.school("b", ["localhost"] * 3, []);
phish.bait.hook("a", 0, phish.bait.DIRECT, 0, "b");

phish.bait.school("c", ["localhost"] * 1, []);
phish.bait.hook("b", 0, phish.bait.SINGLE, 0, "c");

phish.bait.school("d", ["localhost"] * 3, []);
phish.bait.hook("c", 0, phish.bait.ROUND_ROBIN, 0, "d");

phish.bait.school("e", ["localhost"] * 3, []);
phish.bait.hook("d", 0, phish.bait.PAIRED, 0, "e");

phish.bait.school("f", ["localhost"] * 3, []);
phish.bait.hook("e", 0, phish.bait.HASHED, 0, "f");

phish.bait.school("g", ["localhost"] * 3, []);
phish.bait.hook("f", 0, phish.bait.BROADCAST, 0, "g");

phish.bait.school("h", ["localhost"] * 7, []);
phish.bait.hook("h", 0, phish.bait.CHAIN, 0, "h");

phish.bait.school("i", ["localhost"] * 7, []);
phish.bait.hook("i", 0, phish.bait.RING, 0, "i");

phish.bait.start();

