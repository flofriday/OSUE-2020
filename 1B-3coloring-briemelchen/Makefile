# makefile for making supervisor and generator
# author: briemelchen
# last modified: 14.11.2020
CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L   -g  
TARGETS = generator supervisor
LDFLAGS = -lpthread -lrt # needed to link semaphores (-lpthread) /shared mem (-lrt)


all: $(TARGETS)

generator.o: generator.c
	gcc $(CFLAGS) -c $<

supervisor.o: supervisor.c
	gcc $(CFLAGS) -c $<

buffer.o: buffer.c
	gcc $(CFLAGS) -c $<

solver.o: solver.c
	gcc $(CFLAGS) -c $<

supervisor: supervisor.o buffer.o solver.o
	gcc -o $@ $^ $(LDFLAGS)

generator: generator.o buffer.o solver.o
	gcc -o $@ $^ $(LDFLAGS)

clean:
	rm -rf *.o $(TARGETS)
