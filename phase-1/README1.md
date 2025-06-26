# Simple TCP Chat - Phase 1

A minimal terminal-based client-server chat application in C using TCP sockets.  
**Phase 1** demonstrates a single-client connection where the server echoes messages sent by the client.
It is developed just to know whether sockets are working or not!

---

##  Features

- Single client connects to server
- Client sends messages from terminal
- Server echoes messages back
- Clean disconnection handling
- No memory leaks
- Clear logging for connection events

---

##  Files
```
| File       | Description                           |
|------------|---------------------------------------|
| `server.c` | TCP server that listens and echoes    |
| `client.c` | TCP client that sends/receives text   |
| `Makefile` | For building both binaries            |
```
---

## Build Instructions

```bash
make          # Builds both server and client
make clean    # Cleans binaries

Or manually:

gcc server.c -o server
gcc client.c -o client

```

---

## Running The App
```
 # Start the Server
  ./server
 -> By default, it listens on 0.0.0.0:9001.
 # Start the Client (in another terminal)
  ./client
```
