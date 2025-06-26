# Makefile for Phase 4 Chat App

CC = gcc
CFLAGS = -Wall -Wextra -pthread
WIN_CC = x86_64-w64-mingw32-gcc
BIN_DIR = bin

all: $(BIN_DIR)/server $(BIN_DIR)/client

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

$(BIN_DIR)/server: server.c | $(BIN_DIR)
	$(CC) $(CFLAGS) server.c -o $@

$(BIN_DIR)/client: client.c | $(BIN_DIR)
	$(CC) $(CFLAGS) client.c -o $@

win: $(BIN_DIR)/client_win.exe

$(BIN_DIR)/client_win.exe: client.c | $(BIN_DIR)
	$(WIN_CC) -Wall client.c -o $@

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean win

