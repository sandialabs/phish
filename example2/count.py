import phish

counts = {}

def message_callback(message):
  key, value = message
  if key not in counts:
    counts[key] = 0
  counts[key] += value

def closed_callback():
  for key, count in counts.items():
    phish.send((count, key))
  phish.close()

phish.init()
phish.enable_input_port(0, message_callback, closed_callback)
phish.enable_output_port(0)
phish.check()
phish.loop()

