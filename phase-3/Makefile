# Makefile for Multi-Client Chat App (Phase 3)
# Supports:
# - server      : POSIX multithreaded chat server
# - client      : POSIX client (Linux/macOS)
# - winclient   : Windows client built on Linux using MinGW

CC = gcc
CFLAGS = -Wall -Wextra -g -pthread
WINCC = x86_64-w64-mingw32-gcc
WINFLAGS = -Wall -Wextra -g -lws2_32

TARGETS = server client

all: $(TARGETS)

server: server.c
	$(CC) $(CFLAGS) -o server server.c

client: client.c
	$(CC) $(CFLAGS) -o client client.c

winclient: client.c
	$(WINCC) client.c -o client.exe $(WINFLAGS)

clean:
	rm -f server client client.exe

