CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2
OBJS = server.o admin.o commands.o mute.o log.o banlist.o utils.o

all: server client

server: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

client: client.c
	$(CC) $(CFLAGS) -o $@ client.c

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o server client

.PHONY: all clean
