# makefile for making mycompress
# author: briemelchen
CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g 

all: mycompress


%: %.c
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf mycompress mycompress.o
