CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -g -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -Iinclude
LIBS = -lsodium

# Default target
all: test

# Build and run the sanity tests
test: tests/test_sanity.c
	$(CC) $(CFLAGS) tests/test_sanity.c $(LIBS) -o test_runner
	@echo "--- Running Sanity Tests ---"
	./test_runner

# Run Valgrind memory check on the tests
memcheck: test
	@echo "--- Running Valgrind Memory Check ---"
	valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./test_runner	

# Clean up build artifacts
clean:
	rm -f test_runner gdev-vpn-client gdev-vpn-server *.o

# Mark targets as phony to avoid conflicts with files of the same name
.PHONY: all test memcheck clean

