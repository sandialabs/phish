import optparse
import phish
import random
import sys

sys.argv = phish.init(sys.argv)
my_id = phish.query("idlocal", 0, 0)

parser = optparse.OptionParser()
parser.add_option("--message-size", "-m", type="int", default=80, help="Message size.  Default: %default.")
parser.add_option("--probability", "-p", type="float", default=0.05, help="Probability that a received message will trigger sending an additional message.  Default: %default.")
parser.add_option("--seed", type="int", default=my_id, help="Random seed.  Default: %default.")
options, arguments = parser.parse_args()

message = "-" * options.message_size
random.seed(options.seed)

def source_message(field_count):
  content = phish.unpack()[1]
  phish.pack_int32(my_id)
  phish.pack_string(content)
  phish.send(0)

def loop_message(field_count):
  recipient_id = phish.unpack()[1]
  content = phish.unpack()[1]

  #print my_id, content

  if content == "stop":
    options.probability = 0
    if recipient_id == my_id:
      phish.close(0)

  if recipient_id == my_id:
    return

  phish.pack_int32(recipient_id)
  phish.pack_string(content)
  phish.send(0)

  if random.random() < options.probability:
    phish.pack_int32(my_id)
    phish.pack_string(message)
    phish.send(0)

def loop_done():
  phish.close(0)

phish.input(0, source_message, None, True)
phish.input(1, loop_message, loop_done, True)
phish.output(0)
phish.check()
phish.loop()
phish.exit()
