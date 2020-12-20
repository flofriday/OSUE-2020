# Author: Tobias de Vries (01525369)
# Created: 20.11.2020
# Programs: supervisor & generator

CC      = gcc
DEFS    = -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L
CFLAGS  = -std=c99 -pedantic -Wall -g $(DEFS)
LDFLAGS = -pthread -lrt

G_OBJECTS = generator.o util.o 3color.o
S_OBJECTS = supervisor.o util.o 3color.o

TARGET_COMPILATION = $(CC) -o $@ $^ $(LDFLAGS)
OBJECT_COMPILATION = $(CC) $(CFLAGS) -c -o $@ $<

.PHONY: all clean
all: supervisor generator

generator: $(G_OBJECTS)
	$(TARGET_COMPILATION)

supervisor: $(S_OBJECTS)
	$(TARGET_COMPILATION)

%.o: %.c
	$(OBJECT_COMPILATION)

clean:
	rm -rf *.o supervisor generator
