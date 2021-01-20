# @file Makefile
# @author flofriday <eXXXXXXXX@student.tuwien.ac.at>
# @date 19.12.2020
#
# @brief The Makefile for the client and the server.

CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS) -fdiagnostics-color=always
LDFLAGS = -lrt -lz

SERVER_OBJECTS = server.o
CLIENT_OBJECTS = client.o

.PHONY: all clean release server-test client-test
all: client server

server: $(SERVER_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

client: $(CLIENT_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf *.o server client HW3.tgz

# Create the archive to submit 
release:
	tar -cvzf HW3.tgz Makefile *.c *.py

# Run the server testsuite
server-test:
	python3 servertest.py

# Run the client testsuite
client-test:
	python3 clienttest.py