import phish

def message_callback(message):
  phish.send(message)

def closed_callback():
  phish.close()

phish.init()
phish.enable_input_port(0, message_callback, closed_callback)
phish.enable_output_port(0)
phish.check()
phish.loop()

