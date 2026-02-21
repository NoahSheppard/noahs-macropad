CC := gcc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -O2
LDFLAGS :=

SRC := tetris.c
BIN := build/tetris

.PHONY: all debug run clean

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) $< -o $@ $(LDFLAGS)

debug: CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -g -O0
debug: clean $(BIN)

run: $(BIN)
	./$(BIN)

clean:
	rm -f $(BIN)
