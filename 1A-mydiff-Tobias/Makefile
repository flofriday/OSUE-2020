# Author: Tobias de Vries (01525369)
# Created: 20.11.2020
# Programs: mydiff

CC      = gcc
DEFS    = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS  = -std=c99 -pedantic -Wall -g $(DEFS)

.PHONY: all clean
all: mydiff

mydiff: mydiff.o
	$(CC) -o $@ $^

mydiff.o: mydiff.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o mydiff
