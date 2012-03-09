import optparse
import phish.school

parser = optparse.OptionParser()
parser.add_option("--count", type="int", default=1000, help="Number of pings.  Default: %default.")
parser.add_option("--ping-host", default="localhost", help="Ping host address.  Default: %default.")
parser.add_option("--pong-host", default="localhost", help="Pong host address.  Default: %default.")
parser.add_option("--ping-command", default="zmq-ping", help="Command to start the ping minnow.  Default: %default.")
parser.add_option("--pong-command", default="zmq-pong", help="Command to start the pong minnow.  Default: %default.")
parser.add_option("--size", type="int", default=0, help="Number of bytes in each ping.  Default: %default.")
(options, arguments) = parser.parse_args()

ping = phish.school.create_minnows("ping", options.ping_command.split() + ["--count", str(options.count), "--size", str(options.size)], 1, options.ping_host)
pong = phish.school.create_minnows("pong", options.pong_command.split(), 1, options.pong_host)
phish.school.one_to_one(ping, 0, 0, pong)
phish.school.one_to_one(pong, 0, 0, ping)
phish.school.start()

