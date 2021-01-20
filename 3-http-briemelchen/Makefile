# makefile for making client and server
# author: briemelchen
# last modified: 03.01.2021
CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L   -g  
TARGETS = client server
LDFLAGS = -lz


all: $(TARGETS)

client.o: client.c
	gcc $(CFLAGS) -c $<

util.o: util.c
	gcc $(CFLAGS) -c $<

gziputil.o: gziputil.c
	gcc $(CFLAGS) -c $<


server.o: server.c
	gcc $(CFLAGS) -c $<

server: server.o util.o gziputil.o
	gcc -o $@ $^ $(LDFLAGS)

client: client.o util.o gziputil.o
	gcc -o $@ $^ $(LDFLAGS)

clean:
	rm -rf *.o $(TARGETS)
