#!/usr/local/bin/python

import optparse
import phish
import sys

sys.argv = phish.init(sys.argv)

parser = optparse.OptionParser()
options, arguments = parser.parse_args()

my_id = phish.query("idlocal", 0, 0)

def source_message(field_count):
  content = phish.unpack()[1]
  if content == "done":
    phish.close(0)
    return
  phish.pack_int32(my_id)
  phish.pack_string(content)
  phish.send(0)

def loop_message(field_count):
  recipient_id = phish.unpack()[1]
  content = phish.unpack()[1]
  if recipient_id == my_id:
    return
  phish.pack_int32(recipient_id)
  phish.pack_string(content)
  phish.send(0)

def loop_done():
  phish.close(0)

phish.input(0, source_message, None, True)
phish.input(1, loop_message, loop_done, True)
phish.output(0)
phish.check()
phish.loop()
phish.exit()
