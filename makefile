
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
LDLIBS = -lreadline

SRC = $(wildcard src/*.c)
TARGET = shellforge

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(LDLIBS) -o $(TARGET)

test: $(TARGET)
	@set -eu; \
	output=$$(printf 'echo SF_TEST_OK\nexit\n' | ./$(TARGET)); \
	printf '%s\n' "$$output" | grep -q 'SF_TEST_OK'; \
	echo "PASS: Basic command"; \
	output=$$(printf 'echo shellforge | tr a-z A-Z\nexit\n' | ./$(TARGET)); \
	printf '%s\n' "$$output" | grep -q 'SHELLFORGE'; \
	echo "PASS: Pipeline"; \
	tmp=$$(mktemp); \
	trap 'rm -f "$$tmp"' EXIT; \
	output=$$(printf 'echo SF_REDIRECT > %s\ncat %s\nexit\n' "$$tmp" "$$tmp" | ./$(TARGET)); \
	printf '%s\n' "$$output" | grep -q 'SF_REDIRECT'; \
	echo "PASS: Output redirection"; \
	output=$$(printf 'echo SF_HISTORY\nhistory\nexit\n' | ./$(TARGET)); \
	printf '%s\n' "$$output" | grep -q 'SF_HISTORY'; \
	echo "PASS: Command history"

clean:
	rm -f $(TARGET)
