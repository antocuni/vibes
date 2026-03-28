CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -g -O2

all: test_io

test_io: test_io.c spy_io.c spy_io.h
	$(CC) $(CFLAGS) -o $@ test_io.c spy_io.c

test: test_io
	./test_io

clean:
	rm -f test_io /tmp/spy_io_test.tmp

.PHONY: all test clean
