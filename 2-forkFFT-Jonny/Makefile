# @file Makefile
# @author Jonny X (12345678) <e12345678@student.tuwien.ac.at>
# @date 17.12.2020
#
# @brief The makefile which compiles and links files
# 

CC = gcc
DEFS = -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS = -Wall -g -std=c99 -pedantic $(DEFS)
LDFLAGS = -lm
FORKFFT_OBJECTS = forkFFT.o

.PHONY: all clean
all: forkFFT

forkFFT: $(FORKFFT_OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ 

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# generates the tgz file with all .c and .h files
tar:
	tar -cvzf Task2.tgz Makefile *.c

clean:
	rm -rf *.o forkFFT Task2.tgz