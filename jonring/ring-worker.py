import optparse
import phish
import random
import os.path
import Queue
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

file_queue = Queue.Queue()

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
    while True:
      file_index = input_file.readline()
      if file_index != "":
        break
    while True:
      recipient_id = input_file.readline()
      if recipient_id != "":
        break
    recipient_id = int(recipient_id)
    while True:
      content = input_file.readline()
      if content != "":
        break
    content = content.strip()
    file_queue.put((file_index, recipient_id, content))
    #sys.stderr.write("put %s\n" % file_index)

if my_id == head_id:
  thread = threading.Thread(target = read_file)
  thread.daemon = True
  thread.start()

output_file = None
file_index = 0
def send_message(recipient_id, content):
  if my_id == tail_id:
    global output_file, file_index
    if output_file is None:
      output_file = open(options.file, "w+")
    output_file.write("%s\n%s\n%s\n" % (file_index, recipient_id, content))
    file_index += 1
    output_file.flush()
  else:
    phish.pack_int32(recipient_id)
    phish.pack_string(content)
    phish.send(0)

def source_message(field_count):
  content = phish.unpack()[1]
  #sys.stderr.write("%s %s\n" % (my_id, content))
  send_message(my_id, content)

def handle_loop_message(recipient_id, content):
  #sys.stderr.write("%s %s %s\n" % (my_id, recipient_id, content))

  if content == "stop":
    options.probability = 0
    if recipient_id == my_id:
      phish.close(0)

  if recipient_id == my_id:
    return

  send_message(recipient_id, content)

  if random.random() < options.probability:
    send_message(my_id, message)

def loop_message(field_count):
  handle_loop_message(phish.unpack()[1], phish.unpack()[1])

def loop_done():
  phish.close(0)

phish.input(0, source_message, None, True)
phish.input(1, loop_message, loop_done, True)
phish.output(0)
phish.check()

while True:
  try:
    while True:
      file_index, recipient_id, content = file_queue.get_nowait()
      #sys.stderr.write("%s %s %s %s\n" % (my_id, file_index, recipient_id, content))
      handle_loop_message(recipient_id, content)
  except:
    pass
  if -1 == phish.recv():
    break

phish.exit()
