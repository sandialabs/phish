import optparse
import phish
import random
import os.path
import sys
import time
import threading

sys.argv = phish.init(sys.argv)

head_id = 0
tail_id = phish.query("nlocal", 0, 0) - 1
my_id = phish.query("idlocal", 0, 0)

parser = optparse.OptionParser()
parser.add_option("--file", "-f", default=os.path.join(os.getcwd(), "buffer.txt"), help="Buffer file.  Default: %default.")
parser.add_option("--message-size", "-m", type="int", default=80, help="Message size.  Default: %default.")
parser.add_option("--probability", "-p", type="float", default=0.05, help="Probability that a received message will trigger sending an additional message.  Default: %default.")
parser.add_option("--seed", type="int", default=my_id, help="Random seed.  Default: %default.")
options, arguments = parser.parse_args()

message = "-" * options.message_size
random.seed(options.seed)

def read_file():
  try:
    os.remove(options.file)
  except:
    pass

  while True:
    try:
      input_file = open(options.file, "r")
      break
    except:
      time.sleep(0.1)

  while True:
    recipient_id = input_file.readline()
    if recipient_id == "":
      break
    recipient_id = int(recipient_id)
    content = input_file.readline()
    if content == "":
      break
    content = content.strip()
    sys.stderr.write("%s %s\n" % (recipient_id, content))

if my_id == head_id:
  thread = threading.Thread(target = read_file)
  thread.start()

output_file = None
def send_message(recipient_id, content):
  if my_id == tail_id:
    global output_file
    if output_file is None:
      output_file = open(options.file, "w+")
    output_file.write("%s\n%s\n" % (recipient_id, content))
    output_file.flush()
  else:
    phish.pack_int32(recipient_id)
    phish.pack_string(content)
    phish.send(0)

def source_message(field_count):
  content = phish.unpack()[1]
  send_message(my_id, content)

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

  send_message(recipient_id, content)

  if random.random() < options.probability:
    send_message(my_id, message)

def loop_done():
  phish.close(0)

phish.input(0, source_message, None, True)
phish.input(1, loop_message, loop_done, True)
phish.output(0)
phish.check()

while True:
  if -1 == phish.recv():
    break

phish.exit()
