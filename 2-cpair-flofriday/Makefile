# @file Makefile
# @author flofriday <eXXXXXXXX@student.tuwien.ac.at>
# @date 27.11.2020
#
# @ brief The Makefile for cpair

CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS) -fdiagnostics-color=always
LDFLAGS = -lrt -pthread -lm

OBJECTS = cpair.o

.PHONY: all clean release
all: cpair

cpair: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o cpair HW2.tgz

# Create the archive to submit 
release:
	tar -cvzf HW2.tgz Makefile *.c