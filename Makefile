CC = gcc
CFLAGS = -Wall -Wextra -pthread -O2
LDFLAGS = -lssl -lcrypto

# Object files
OBJ_SERVER = server.o admin.o commands.o mute.o log.o banlist.o utils.o motd.o party.o lastseen.o vanish.o pwdgen.o game.o crypto_utils.o ratelimit.o db.o
OBJ_CLIENT = client.o emoji.o crypto_utils.o e2ee.o file_transfer.o

TARGET_SERVER = server
TARGET_CLIENT = client

all: $(TARGET_SERVER) $(TARGET_CLIENT)

$(TARGET_SERVER): $(OBJ_SERVER)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(TARGET_CLIENT): $(OBJ_CLIENT)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o $(TARGET_SERVER) $(TARGET_CLIENT)
