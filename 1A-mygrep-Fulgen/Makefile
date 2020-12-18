# @file Makefile
# @author George Tokmaji <e11908523@student.tuwien.ac.at>
# @date 22.11.2020
#
# @brief Program name: mygrep

CC ?= gcc
CFLAGS = -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -g
DEBUG_FLAGS = -Werror

.PHONY: all clean
all: mygrep

mygrep: mygrep.o
	$(CC) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) *.o mygrep
