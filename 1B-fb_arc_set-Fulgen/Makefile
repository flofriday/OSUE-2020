# @file Makefile
# @author George Tokmaji <e11908523@student.tuwien.ac.at>
# @date 22.11.2020
#
# @brief Makefile
#
# Program names: generator, supervisor

C ?= gcc
CFLAGS = -std=c99 -pedantic -Wall -D_DEFAULT_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809L -DNDEBUG -g

LDLIBS = -lpthread -lrt

.PHONY: all clean
all: generator supervisor

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<
generator: src/common/named_semaphore.o src/common/shared_memory.o src/generator/generator.o
	$(CC) $(LDLIBS) -o $@ $^

supervisor: src/common/named_semaphore.o src/common/shared_memory.o src/supervisor/supervisor.o
	$(CC) $(LDLIBS) -o $@ $^

clean:
	$(RM) *.o src/*/*.o test/*.o libcommon.a supervisor generator
