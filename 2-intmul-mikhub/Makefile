# config

CC = gcc
DEFS = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
override CFLAGS += -pedantic -Wall -std=c99 -g $(DEFS)
override LDLIBS += -lm

TARGET = intmul

# rules

.PHONY : all clean

all: $(TARGET)

%: %.o
	$(CC) -o $@ $^ $(LDLIBS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
    
clean:
	rm -rf $(TARGET) *.o

# dependencies

intmul.o: intmul.c intmul.h