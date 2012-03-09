import phish
import sys

phish.init()
phish.enable_output_port(0)
phish.check()

for path in sys.argv[1:]:
  phish.send(path)

phish.close()
