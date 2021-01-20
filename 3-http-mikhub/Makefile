# config

CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
override CFLAGS += -pedantic -Wall -std=c99 -g $(DEFS)

TARGETS = client server

# rules

.PHONY : all clean

all: $(TARGETS)

%: %.o
	$(CC) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
    
clean:
	rm -rf $(TARGETS) *.o

# dependencies

client: client.o utils.o
server: server.o utils.o

server.o: server.c server.h
client.o: client.c client.h
utils.o: utils.c utils.h