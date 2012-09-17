#!/usr/local/bin/python

# MINNOW filegen
# emit filenames from list of files & dirs, scanned recursively

import os,sys
import phish

argv = phish.init(sys.argv)
phish.output(0)
phish.check()

files = []
for path in argv[1:]:
  if os.path.isdir(path):
    for directory, directories, files in os.walk(path):
      files += [os.path.join(directory, file) for file in files]
  else:
    files.append(path)

for file in files:
  phish.pack_string(file)
  phish.send(0)

phish.exit()
