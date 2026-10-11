CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -Werror -O2 -g -D_POSIX_C_SOURCE=200809L -D_DEFAULT_SOURCE -Iinclude
LIBS = -lsodium

# Auto-detect source files in the src directory, excluding any *_main.c files
SRCS = $(filter-out src/%_main.c, $(wildcard src/*.c))
OBJS = $(patsubst src/%.c, build/%.o, $(SRCS))

# Auto-detect test source files in the tests directory
TEST_SRCS = $(wildcard tests/test_*.c)
TEST_BINS = $(patsubst tests/%.c, build/%, $(TEST_SRCS))

# Default target
all: test

# Create the build directory if it doesn't exist
build:
	mkdir -p build

# Compile source files into object files
build/%.o: src/%.c | build
	$(CC) $(CFLAGS) -c $< -o $@

# Link test binaries from test source files
build/%: tests/%.c $(OBJS) | build
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

# Build and run the sanity tests
test: $(TEST_BINS)
	@echo "=== RUNNING ALL TESTS ==="
	@for t in $(TEST_BINS); do \
		echo "--> Running $$t"; \
		./$$t || exit 1; \
	done
	@echo "=== ALL TESTS PASSED ==="

# Run Valgrind memory check on the tests
memcheck: $(TEST_BINS)
	@echo "=== RUNNING VALGRIND FOR ALL TESTS ==="
	@for t in $(TEST_BINS); do \
		echo "--> Valgrind on $$t"; \
		valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./$$t || exit 1; \
	done
	@echo "=== ALL MEMMORY CHECKS PASSED ==="

# Clean up build artifacts
clean:
	rm -rf build gdev-vpn-client gdev-vpn-server *.o

# Mark targets as phony to avoid conflicts with files of the same name
.PHONY: all test memcheck clean

