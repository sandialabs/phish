# Makefile for PHISH minnows

SHELL = /bin/bash

# Definitions
# auto-generates list of minnows from *.cpp and *.c files that contain MINNOW
# NOTE: what if additional C++ files needed to build a minnow?
#       need way to add minnow-specific rules

SRCCPP = 	$(shell grep -l MINNOW *.cpp)
MINNOWCPP =	$(SRCCPP:.cpp=)
SRCC = 		$(shell grep -l MINNOW *.c)
MINNOWC =	$(SRCC:.c=)
MINNOW = 	$(MINNOWCPP) $(MINNOWC)

# List of all targets

help:
	@echo ''
	@echo 'make clean	        delete object files'
	@echo 'make clean-all           delete object and executable files'
	@echo ''
	@echo 'make machine             build Minnows where machine is one of:'
	@echo ''
	@files="`ls Makefile.*`"; \
	  for file in $$files; do head -1 $$file; done
	@echo ''

# Build minnows

.DEFAULT:
	@for file in $(MINNOW); do $(MAKE) -f Makefile.$@ \
	"EXE = $$file" $$file; done

# Clean up

clean:
	@rm -f *.o

clean-all:
	@rm -f *.o
	@rm -f $(MINNOW)
