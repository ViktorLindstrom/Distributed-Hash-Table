# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11

# Targets
all: testsystem test_insert test_lookup_new

testsystem: test_lookup.c
	$(CC) $(CFLAGS) -o test_lookup test_lookup.c

test_insert: test_insert.c
	$(CC) $(CFLAGS) -o test_insert test_insert.c

test_lookup_new: test_new_lookup.c
	$(CC) $(CFLAGS) -o test_lookup1 test_new_lookup.c

clean:
	rm -f test_lookup test_insert test_lookup1
