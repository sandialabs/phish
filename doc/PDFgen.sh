#!/bin/csh
# generate a PDF version of Manual

txt2html -b *.txt

htmldoc --title --toctitle "Table of Contents" --tocfooter ..i --toclevels 4 --header ... --footer ..1 --size letter --linkstyle plain --linkcolor blue -f Manual.pdf Manual.html Section_intro.html Section_bait.html Section_minnows.html Section_library.html Section_examples.html Section_python.html Section_errors.html [a-z]*.html

txt2html *.txt
