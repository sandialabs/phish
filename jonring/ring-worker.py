import optparse
import phish
import random
import sys

sys.argv = phish.init(sys.argv)
my_id = phish.query("idlocal", 0, 0)
worker_count = phish.query("nlocal", 0, 0)

parser = optparse.OptionParser()
parser.add_option("--ticks", "-t", type="int", default=20, help="Number of simultaneous ticks.  Default: %default.")
parser.add_option("--message-size", "-m", type="int", default=80, help="Message size.  Default: %default.")
parser.add_option("--seed", type="int", default=my_id, help="Random seed.  Default: %default.")
options, arguments = parser.parse_args()

message = "-" * options.message_size
random.seed(options.seed)
stopped = False

if my_id == 0:
  for n in range(options.ticks):
    phish.pack_int32(my_id)
    phish.pack_string("tick")
    phish.send(2)

def source_message(field_count):
  content = phish.unpack()[1]

  #print "worker", my_id, content

  phish.pack_int32(my_id)
  phish.pack_string(content)
  phish.send(2)

def loop_message(field_count):
  recipient_id = phish.unpack()[1]
  content = phish.unpack()[1]

  #print "worker", my_id, content

  if content == "tick":
    tick_message(recipient_id, content)
  elif content == "stop":
    stop_message(recipient_id, content)
  else:
    content_message(recipient_id, content)

def tick_message(recipient_id, content):
  global stopped
  if stopped:
    return

  if recipient_id == my_id:
    phish.pack_string(content)
    phish.send(1)

  phish.pack_int32(recipient_id)
  phish.pack_string(content)
  phish.send(2)

def content_message(recipient_id, content):
  if recipient_id == my_id:
    return

  phish.pack_int32(recipient_id)
  phish.pack_string(content)
  phish.send(2)

def stop_message(recipient_id, content):
  global stopped
  stopped = True

  print "worker", my_id, content

  if recipient_id == my_id:
    return

  phish.pack_int32(recipient_id)
  phish.pack_string(content)
  phish.send(2)
  phish.close(1)
  phish.close(2)

phish.input(0, source_message, None, True)
phish.input(2, loop_message, None, True)
phish.output(1)
phish.output(2)
phish.check()
phish.loop()
phish.exit()
