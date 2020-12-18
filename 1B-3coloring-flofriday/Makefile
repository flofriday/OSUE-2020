# @file Makefile
# @author flofriday <eXXXXXXXX@student.tuwien.ac.at>
# @date 31.10.2020
#
# @brief The Makefile for supervisor and generator program.

CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS) -fdiagnostics-color=always
LDFLAGS = -lrt -pthread

GENERATOR_OBJECTS = generator.o sharedmem.o circbuf.o
SUPERVISOR_OBJECTS = supervisor.o sharedmem.o circbuf.o

.PHONY: all all-slow clean release format
all: generator supervisor

# Compile with the slow algorithm for coloring (as described in the assignment).
# Note: before use clean object files with make clean
all-slow: CFLAGS += -DSLOW_ALGO
all-slow: all

generator: $(GENERATOR_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

supervisor: $(SUPERVISOR_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o generator supervisor HW1B.tgz

# Create the archive to submit 
release:
	tar -cvzf HW1B.tgz Makefile *.c *.h