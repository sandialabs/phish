import optparse
import phish

items = []

def message_callback(message):
  global items
  items.append(message)

def closed_callback():
  global items
  for item in sorted(items, reverse=True)[:options.count]:
    phish.send(item)
  phish.close()

phish.init()
phish.enable_input_port(0, message_callback, closed_callback)
phish.enable_output_port(0)
phish.check()

parser = optparse.OptionParser()
parser.add_option("--count", type="int", default=10, help="Maxium number of items to return.  Default: %default.")
(options, arguments) = parser.parse_args()

phish.loop()

