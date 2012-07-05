# Demonstrates using the Python bindings to the bait API to programmatically
# setup a Phish job.
#
# Uses the graphviz backend to generate a dotfile that can be processed with
# graphviz to display a diagram of the job.  If this were a real job, you would
# replace "import bait.graphviz" with "import bait.mpi" or "import bait.zmq" to
# do the actual computation.

import bait.graphviz

bait.minnows("a", ["localhost"] * 1, []);

bait.minnows("b", ["localhost"] * 3, []);
bait.hook("a", 0, bait.DIRECT, 0, "b");

bait.minnows("c", ["localhost"] * 1, []);
bait.hook("b", 0, bait.SINGLE, 0, "c");

bait.minnows("d", ["localhost"] * 3, []);
bait.hook("c", 0, bait.ROUND_ROBIN, 0, "d");

bait.minnows("e", ["localhost"] * 3, []);
bait.hook("d", 0, bait.PAIRED, 0, "e");

bait.minnows("f", ["localhost"] * 3, []);
bait.hook("e", 0, bait.HASHED, 0, "f");

bait.minnows("g", ["localhost"] * 3, []);
bait.hook("f", 0, bait.BROADCAST, 0, "g");

bait.minnows("h", ["localhost"] * 7, []);
bait.hook("h", 0, bait.CHAIN, 0, "h");

bait.minnows("i", ["localhost"] * 7, []);
bait.hook("i", 0, bait.RING, 0, "i");

bait.start();

