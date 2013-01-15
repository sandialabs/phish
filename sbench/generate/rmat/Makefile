INCLUDE = -I./
#LINK= -lpcap
prods = udpgen
CC=gcc
CFLAGS += $(INCLUDE) -Wall -O3 -std=gnu99 -I ../../include
CFLAGS += $(objinc)

all: $(prods)

udpgen: udpgen.o
	$(CC) $(CFLAGS) $< -o $@ $(LINK) 

.o: %.c

clean:
	rm -f $(prods) *.o
