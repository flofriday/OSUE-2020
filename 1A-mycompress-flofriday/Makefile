# @file Makefile
# @author flofriday XXXXXXXX <eXXXXXXXX@student.tuwien.ac.at>
# @date 21.10.2020
#
# @brief Makefile for mycompress

CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)

OBJECTS = main.o 

.PHONY: all clean
all: mycompress

mycompress: $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

main.o: main.c 

clean:
	rm -rf *.o mycompress HW1A.tgz

# Create the archive to upload to TUWEL
release:
	tar -cvzf HW1A.tgz main.c Makefile