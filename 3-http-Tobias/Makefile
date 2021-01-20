# Author: Tobias de Vries (01525369)
# Created: 29.12.2020
# Programs: server

CC      = gcc
DEFS    = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS  = -std=c99 -pedantic -Wall -g $(DEFS)

.PHONY: all clean
all: client server

server: server.o
	$(CC) -o $@ $^

server.o: server.c
	$(CC) $(CFLAGS) -c -o $@ $<

client: client.o
	$(CC) -o $@ $^

client.o: client.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o client server
