# Advance Terminal Chat App in C

### This is a terminal-based chat application written in C. It allows many users to connect to a server and chat in real-time. It uses TCP sockets and threads (pthread) to support multiple clients at once. The app runs on Linux.
This all is under developement here is my plan and criteria of developement.
---

## Technology Used
```
| Part            | Tool / Library            |
|------------------|---------------------------|
| Language         | C (GCC / Clang)           |
| Networking       | TCP/IP sockets (IPv4)     |
| Threads          | POSIX Threads (`pthread`) |
| I/O              | Blocking for now, later non-blocking with `select()` |
| Security (later) | OpenSSL for TLS           |
| Operating System | Linux(prefered)           |
| Build Tool       | `make` and `gcc`          |
```
---

##  Project Files

```
chat-app/
├── server.c #server code
├── client.c # Client code
├── Makefile # Build script
├── .gitignore # Files to ignore in Git
├── Dockerisation and CI/CD via jenkins and kubernetes with AWS
└── README.md # This file


```
---

## Development Plan

```bash
| Phase   | What it will do                                                   |
|---------|-------------------------------------------------------------------|
|    1    | Chat between one server and one client                            |
|    2    | Support many clients using threads                                |
|    3    | Messages are sent to all clients (broadcast)                      |
|    4    | Ask users for a username, show when they join or leave            
|    5    | Support commands like `/quit` or `/msg` (private messages)        |
|    6    | Use `select()` for better performance with many users             |
|    7    | Add security using TLS (encrypt messages)                         |
|    8    | Server logs all messages and actions with timestamps              |
|    9    | Support private messages between users                            |
|    10   | Add a Dockerfile to run server/client easily                      |
|    11   | Make it work on Windows too (optional)                            |
|    12   | Run it using AWS's public IP so that anyone can join.             |
```

---


---


##  Planned Chat Commands (coming soon)

   ```bash

    /quit → Leave the chat

    /msg <username> → Send private message

    /whoami → Show your name

    /help → Show commands

    /clear → Clear screen
```

---

## Security Plans

    -> Use TLS to encrypt all chat messages

    -> Make it harder for attackers to read messages

    -> (Optional) Add passwords to join server

---
---
## Other Features to Add Later

    :-> Add timestamps to messages

    :-> Limit number of users

    :-> Try to reconnect if a client disconnects

    :-> Maybe send files or emojis (optional)
---


##  About the Developer

**[NEXUS8222](https://github.com/nexus8222)** — Part time System Dev, Full time Network Engineer and Cyber security Enthusiast.  

### Remeber me? Or i need to invoke you:)

