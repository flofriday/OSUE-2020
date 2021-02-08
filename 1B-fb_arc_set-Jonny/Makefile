# @file Makefile
# @author Jonny X (12345678) <e12345678@student.tuwien.ac.at>
# @date 19.11.2020
#
# @brief The makefile which compiles and links files
# 

CC = gcc
DEFS = -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L

CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)
LDFLAGS = -lrt -pthread

SUPERVISOR_OBJECTS = supervisor.o circular_buffer.o
GENERATOR_OBJECTS = generator.o circular_buffer.o

all-fast: CFLAGS += -Ofast
all-fast: all

.PHONY: all clean
all: supervisor generator

supervisor: $(SUPERVISOR_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ 

generator: $(GENERATOR_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# generates the tgz file with all .c and .h files
tar:
	tar -cvzf Task1B.tgz Makefile *.c *.h

clean:
	rm -rf *.o supervisor generator Task1B.tgz