# config

DEFS = -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809
override CFLAGS += -Wall -g -std=c99 -pedantic $(DEFS)
override LDFLAGS +=
override LIBS += -lrt -lpthread

# rules

.PHONY : all clean

all: generator supervisor

generator: generator.o
	gcc $(LDFLAGS) -o $@ $^ $(LIBS)

supervisor: supervisor.o
	gcc $(LDFLAGS) -o $@ $^ $(LIBS)

%.o: %.c
	gcc $(CFLAGS) -c -o $@ $<

clean:
	rm -f generator generator.o supervisor supervisor.o

# dependencies

generator.o: generator.c fb_arc_set.h
supervisor.o: supervisor.c fb_arc_set.h
