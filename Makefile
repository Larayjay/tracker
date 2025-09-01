CC=clang
CFLAGS=-Wall -Wextra -std=c17
SRC=src/main.c
BIN=tracker

all:
	$(CC) $(CFLAGS) $(SRC) -o $(BIN)

run: all
	./$(BIN)

clean:
	rm -f $(BIN)
