import phish

def message_callback(message):
  for word in open(message, "r").read().split():
    phish.send((word, 1))

def closed_callback():
  phish.close()

phish.init()
phish.enable_input_port(0, message_callback, closed_callback)
phish.enable_output_port(0)
phish.check()
phish.loop()

