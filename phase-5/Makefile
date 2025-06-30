CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2

SERVER_OBJ = server.o admin.o commands.o mute.o log.o banlist.o utils.o motd.o
CLIENT_OBJ = client.o

TARGETS = server client

all: $(TARGETS)

server: $(SERVER_OBJ)
	$(CC) $(CFLAGS) -o server $(SERVER_OBJ)

client: $(CLIENT_OBJ)
	$(CC) $(CFLAGS) -o client $(CLIENT_OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o $(TARGETS)
