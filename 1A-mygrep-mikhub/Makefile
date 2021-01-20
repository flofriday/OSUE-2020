# @file Makefile
# @author Michael Huber 11712763
# @date 10.11.2020

# config

DEFS = -D DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
override CFLAGS += -pedantic -Wall -std=c99 -g $(DEFS)

# target and object files

TARGET = mygrep
OBJS = mygrep.o

# rules

.PHONY : all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	gcc $(CFLAGS) -o $@ $^

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $<
    
clean:
	rm -rf $(TARGET) $(OBJS)

# dependencies

mygrep.o: mygrep.c mygrep.h