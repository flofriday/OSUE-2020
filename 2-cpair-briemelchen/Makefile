# makefile for making cpair
# author: briemelchen
# last modified: 14.11.2020
CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L   -g  
TARGETS = cpair
LDFLAGS = -lm #math.h

all: $(TARGETS)

cpair.o: cpair.c
	gcc $(CFLAGS) -c $<

cpair: cpair.o 
	gcc -o $@ $^  $(LDFLAGS)

clean:
	rm -rf *.o $(TARGETS)
