import optparse
import phish
import time

def message_callback(message):
  global start_time
  global delta_time
  current_time = time.clock()
  if current_time - start_time < delta_time:
    time.sleep(delta_time - (current_time - start_time))
  phish.send(message)
  start_time = time.clock()

def closed_callback():
  phish.close()

phish.init()
phish.enable_input_port(0, message_callback, closed_callback)
phish.enable_output_port(0)
phish.check()

parser = optparse.OptionParser()
parser.add_option("--rate", type="float", default=2, help="Rate at which messages should be forwarded, in messages-per-second.  Default: %default.")
(options, arguments) = parser.parse_args()

start_time = time.clock()
delta_time = 1.0 / options.rate

phish.loop()

