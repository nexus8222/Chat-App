# Terminal Chat App 🚀
### Production-Grade Concurrent Terminal Chat Ecosystem in Pure C

[![Language](https://img.shields.io/badge/Language-C11-blue.svg)](https://en.cppreference.com/w/c)
[![Security](https://img.shields.io/badge/Security-OpenSSL%203.0-green.svg)](https://www.openssl.org/)
[![Protocol](https://img.shields.io/badge/Cryptography-Signal%20Double%20Ratchet-blueviolet.svg)](https://signal.org/docs/specifications/doubleratchet/)
[![Build](https://img.shields.io/badge/Build-Passing%20(0%20warnings)-brightgreen.svg)]()
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)]()

A high-performance, multithreaded terminal chat server and client written in pure C using POSIX sockets, `pthread` concurrency, and OpenSSL 3.0. Features **Signal-style Double Ratchet End-to-End Encryption (E2EE)**, **in-band chunked file streaming with SHA-256 integrity verification**, **Token Bucket anti-spam rate limiting**, **PBKDF2-HMAC-SHA256 administrative authentication**, **ephemeral vanishing messages**, **multi-room parties**, and **raw-mode terminal UX enhancements** (Tab auto-completion, history navigation, and live typing indicators).

---

## 📑 Table of Contents
- [Key Features](#-key-features)
- [Architecture & Cryptographic Design](#-architecture--cryptographic-design)
- [Project Structure](#-project-structure)
- [Prerequisites & Dependencies](#-prerequisites--dependencies)
- [Build & Quick Start](#-build--quick-start)
- [Docker Deployment](#-docker-deployment)
- [Command Reference](#-command-reference)
- [Internal Wire Protocol](#-internal-wire-protocol)
- [Automated Verification Suite](#-automated-verification-suite)
- [Developer & License](#-developer--license)

---

## 🌟 Key Features

### 🔒 1. Signal-Style Double Ratchet E2EE
- **End-to-End Encryption**: Zero server-side plaintext visibility.
- **Diffie-Hellman Ratchet**: Curve25519 (`X25519`) dynamic key exchange on alternating rounds.
- **Symmetric Chain Ratchet**: HKDF-SHA256 message and chain key derivations (`WhisperRatchetRoot`, `WhisperRatchetChain`, `WhisperMessageKeys`).
- **Authenticated Encryption**: AES-256-GCM AEAD with 12-byte initialization vectors and 16-byte authentication tags.
- **Forward Secrecy & Break-in Recovery**: Old keys cannot decrypt future messages; past keys cannot be derived from a compromised ephemeral state.
- **Public Key Directory**: Automatic identity key broadcast and directory querying via `__IDKEY__` and `__KEYREQ__`.

### 📁 2. Chunked In-Band File Transfer
- **In-Band Slicing**: Files are split into 1024-byte chunks and streamed via Base64 wire packets.
- **Negotiation Protocol**: Senders issue transfer offers; receivers interactively accept (`/fileaccept <id>`) or decline (`/filedecline <id>`).
- **Integrity Verification**: Automatic SHA-256 checksum comparison on the receiving client before saving into `downloads/`.
- **Live Terminal Progress Bar**: Dynamic ASCII progress bar (`[===========>       ] 60%`) rendered in real-time.

### 🛡️ 3. Anti-Spam Rate Limiting & DoS Defense
- **Token Bucket Algorithm**: 5.0 token capacity with continuous 1.5 tokens/sec replenishment per client.
- **Automated Mute Enforcement**: Warns users when tokens deplete; automatically applies a 15-second mute upon 3 consecutive burst violations.
- **Per-IP Connection Throttling**: Restricts maximum concurrent TCP connections per IP address to mitigate socket-exhaustion DoS attacks.
- **Protocol Fast-Path**: Internal control messages (`__FILECHUNK__`, `__TYPING__`, `__E2EE__`) bypass human rate limits to prevent file stalls.

### 🔐 4. Secure PBKDF2 Authentication & Offline Inbox
- **Cryptographic Storage**: Replaced plaintext files with OpenSSL PBKDF2-HMAC-SHA256 (10,000 iterations, 16-byte cryptographically secure random salt) stored in `admin.cred`.
- **Dynamic Password Management**: Administrators can rotate server passwords at runtime with `/setadminpwd <newpwd>`.
- **Persistent Offline Store**: Private messages to offline users are queued in `offline_inbox.dat` and replayed immediately upon reconnection with original timestamps.

### 👥 5. Multi-Room Party System
- **Isolated Channels**: Create, join, or leave isolated chat rooms (`/createparty`, `/joinparty`, `/leaveparty`).
- **Room Access Control**: Lock rooms with `/lockparty` or invite specific users with `/inviteparty`.

### ⏱️ 6. Vanishing Messages & Mini-Games
- **Self-Destructing Messages**: Send messages that automatically vanish after a specified TTL (`/vanish <sec> <msg>`).
- **Interactive Mini-Games**: Multiplayer number-guessing game with turn cycling and scoreboards (`/game`).

### 💻 7. Modern Terminal UX
- **Tab Autocompletion**: Press `<Tab>` to cycle through slash commands.
- **Command History Navigation**: Browse previously typed commands using Up/Down arrow keys.
- **Live Typing Indicators**: Peers receive real-time notifications when a user is composing a message (`__TYPING__`).
- **ANSI Color Customization**: Customize chat aesthetics on the fly (`/setcolor <color>`).
- **Interactive Emoji Picker**: 80+ categorized UTF-8 emojis accessible via `/emoji`.

---

## 🏛️ Architecture & Cryptographic Design

```
+-----------------------------------------------------------------------------+
|                           CLIENT TERMINAL (Alice)                           |
|  [Raw Mode Termios]  <--->  [Double Ratchet Engine]  <--->  [File Streamer] |
+-----------------------------------------------------------------------------+
                                      |
                     TCP / POSIX Sockets (Port 9001)
                                      |
                                      v
+-----------------------------------------------------------------------------+
|                             CHAT SERVER DAEMON                              |
|  - Token Bucket Rate Limiter          - PBKDF2 Credential Engine            |
|  - Per-IP DoS Connection Guard        - Public Identity Key Directory       |
|  - Multi-Room Party Router            - Offline Message Inbox Store         |
|  - Moderation Arsenal (Kick/Ban/Mute) - Internal Protocol Dispatcher        |
+-----------------------------------------------------------------------------+
                                      |
                     TCP / POSIX Sockets (Port 9001)
                                      |
                                      v
+-----------------------------------------------------------------------------+
|                            CLIENT TERMINAL (Bob)                            |
|  [Raw Mode Termios]  <--->  [Double Ratchet Engine]  <--->  [File Receiver] |
+-----------------------------------------------------------------------------+
```

---

## 📂 Project Structure

```
.
├── Makefile                # Strict compiler build system (-Wall -Wextra -pthread -O2)
├── Dockerfile              # Containerized build & runtime environment
├── docker-compose.yml      # Service orchestration manifest
├── README.md               # Complete project documentation
├── mind.md                 # Full technical specification & architecture logs
├── checklist               # Development tracking roadmap
│
├── server.c                # Master server daemon & socket accept loop
├── client.c                # Production client with raw termios & event multiplexing
│
├── crypto_utils.h / .c     # OpenSSL 3.0 wrappers: X25519, HKDF, AES-256-GCM, PBKDF2
├── e2ee.h / .c             # Signal Double Ratchet protocol implementation
├── file_transfer.h / .c    # In-band chunked file streaming & SHA-256 verification
├── ratelimit.h / .c        # Token bucket rate limiter & IP connection tracker
├── db.h / .c               # PBKDF2 credential storage, offline inbox & key directory
│
├── admin.h / .c            # Moderation controls: /kick, /ban, /unban, /whois
├── commands.h / .c         # User command dispatch table & argument parsing
├── banlist.h / .c          # IP ban list manager with persistent storage
├── mute.h / .c             # User muting logic and mutelist tracking
├── party.h / .c            # Multi-room channel management & room locking
├── vanish.h / .c           # Ephemeral self-destructing message decay thread
├── game.h / .c             # Turn-based number guessing mini-game
├── emoji.h / .c            # 80-item categorized UTF-8 emoji catalog
├── lastseen.h / .c         # Binary serialized user logout tracker
├── log.h / .c              # Timestamped event and audit logger
├── motd.h / .c             # Message of the Day loader and updater
├── pwdgen.h / .c           # Legacy administrative cipher helpers
├── utils.h / .c            # String trimming and socket formatting utilities
├── common.h / client.h     # Shared data structures and protocol constants
│
└── test_e2e_full.c         # Automated socket-level verification suite
```

---

## 🔧 Prerequisites & Dependencies

- **Operating System**: Linux (Ubuntu 20.04+, Debian, Arch, Fedora) or POSIX-compliant environment.
- **Compiler**: GCC 9.0+ or Clang 10.0+ with C11 support.
- **Libraries**:
  - `pthread` (POSIX Threads)
  - `libssl` & `libcrypto` (OpenSSL 3.0+)

### Installing Dependencies on Ubuntu / Debian:
```bash
sudo apt-get update
sudo apt-get install -y build-essential libssl-dev
```

---

## 🚀 Build & Quick Start

### 1. Compile Everything
```bash
make clean && make
```
*Compiles both `server` and `client` binaries with zero warnings under `-Wall -Wextra -pthread -O2`.*

### 2. Start the Server Daemon
```bash
./server
```
- Binds to `0.0.0.0:9001`.
- Initializes PBKDF2 admin credentials in `admin.cred` (default: `admin123`).

### 3. Connect Interactive Clients
Open a new terminal window for each client:

```bash
# Connect as a standard user
./client 127.0.0.1
Enter your username: alice

# Connect as another user
./client 127.0.0.1
Enter your username: bob
```

### 4. Connect as Administrator
```bash
./client 127.0.0.1
Enter your username: admin
Enter Admin Password: admin123
Enter Joining Alias: root_admin
```

---

## 🐳 Docker Deployment

Run the chat server effortlessly using Docker:

```bash
# Build and run the server container
docker compose up --build -d

# View server logs
docker compose logs -f

# Stop the server container
docker compose down
```

---

## 📖 Command Reference

### User & Communication Commands
| Command | Syntax | Description |
|:---|:---|:---|
| `/help` | `/help` | Displays available commands. |
| `/msg` | `/msg <user> <message>` | Sends a private message to a specific user. |
| `/e2ee` | `/e2ee <user> <message>` | Sends a Signal Double Ratchet end-to-end encrypted message. |
| `/sendfile` | `/sendfile <user> <filepath>` | Sends a file via chunked streaming with SHA-256 verification. |
| `/fileaccept` | `/fileaccept <transfer_id>` | Accepts an incoming file transfer offer. |
| `/filedecline` | `/filedecline <transfer_id>` | Declines an incoming file transfer offer. |
| `/vanish` | `/vanish <sec> <message>` | Sends an ephemeral message that self-destructs after `<sec>` seconds. |
| `/users` | `/users` | Lists all currently connected online users. |
| `/whoami` | `/whoami` | Displays your current username, IP, and room code. |
| `/lastseen` | `/lastseen <user>` | Displays the last logout timestamp for a user. |
| `/emoji` | `/emoji [id]` | Displays the emoji palette or sends a selected emoji. |
| `/setcolor` | `/setcolor <color>` | Sets custom ANSI chat color (`red`, `green`, `yellow`, `blue`, `magenta`, `cyan`). |
| `/game` | `/game [guess]` | Starts or plays the turn-based number guessing game. |
| `/time` | `/time` | Prints the server's local date and time. |
| `/uptime` | `/uptime` | Displays server operational uptime. |
| `/clear` | `/clear` | Clears the terminal screen buffer. |
| `/exit` | `/exit` | Gracefully disconnects from the chat server. |

### Multi-Room Party Commands
| Command | Syntax | Description |
|:---|:---|:---|
| `/createparty` | `/createparty` | Creates a new isolated chat room with a random 4-digit code. |
| `/joinparty` | `/joinparty <code>` | Joins an existing party room. |
| `/leaveparty` | `/leaveparty` | Leaves the current party and returns to the public lounge (`0000`). |
| `/partyusers` | `/partyusers` | Lists members inside your current party room. |
| `/lockparty` | `/lockparty` | Locks the party room so only invited members can join. |
| `/inviteparty` | `/inviteparty <user>` | Invites a user into your locked party room. |

### Administrator Commands
| Command | Syntax | Description |
|:---|:---|:---|
| `/kick` | `/kick <user>` | Forcibly disconnects a user's TCP connection. |
| `/ban` | `/ban <user>` / `/ban <ip>` | Bans a user or IP address; persisted in `banned.txt`. |
| `/unban` | `/unban <ip>` | Removes an IP from the ban list. |
| `/banlist` | `/banlist` | Dumps all currently banned IP addresses. |
| `/mute` | `/mute <user>` | Suppresses a user from sending public or private messages. |
| `/unmute` | `/unmute <user>` | Restores a muted user's messaging privileges. |
| `/mutelist` | `/mutelist` | Lists all currently muted operators. |
| `/whois` | `/whois <user>` | Displays user metadata (IP address, party, admin status, session duration). |
| `/broadcast` | `/broadcast <message>` | Sends an administrative broadcast across all rooms. |
| `/pin` | `/pin <message>` | Sets a sticky banner shown to all connecting clients. |
| `/setmotd` | `/setmotd <message>` | Updates the Message of the Day; persisted in `motd.txt`. |
| `/setadminpwd` | `/setadminpwd <newpwd>` | Dynamically updates the server PBKDF2 administrative password. |
| `/log` | `/log` | Streams the contents of `server.log` to the admin terminal. |
| `/shutdown` | `/shutdown` | Gracefully shuts down the server daemon. |

---

## ⚡ Internal Wire Protocol

The server uses an internal protocol router (`handle_internal_protocol`) to differentiate between human chat inputs and subsystem control packets:

| Header | Format | Purpose |
|:---|:---|:---|
| `__IDKEY__` | `__IDKEY__:<pubkey_hex>` | Registers client's Curve25519 identity key on connect. |
| `__KEYREQ__` | `__KEYREQ__:<username>` | Requests a user's public identity key from the directory. |
| `__KEYRESP__` | `__KEYRESP__:<username>:<pubkey_hex>` | Server response containing the requested public identity key. |
| `__E2EE__` | `__E2EE__:<from>:<to>:<ik>:<ratchet>:<seq>:<iv>:<ct>:<tag>` | Double Ratchet encrypted envelope (AES-256-GCM). |
| `__FILEREQ__` | `__FILEREQ__:<id>:<to>:<filename>:<size>:<sha256>` | Proposes an in-band file transfer offer. |
| `__FILEACCEPT__` | `__FILEACCEPT__:<id>:<from>` | Accepts a pending file transfer session. |
| `__FILEDECLINE__` | `__FILEDECLINE__:<id>:<from>` | Declines a pending file transfer session. |
| `__FILECHUNK__` | `__FILECHUNK__:<id>:<target>:<seq>:<total>:<base64>` | Streams a 1024-byte Base64-encoded chunk. |
| `__FILEDONE__` | `__FILEDONE__:<id>:<target>` | Signals completion of transmission. |
| `__TYPING__` | `__TYPING__:<username>` | Broadcasts typing status indicator within the room. |

---

## 🧪 Automated Verification Suite

The repository includes a comprehensive, multi-client socket-level integration test suite in [`test_e2e_full.c`](test_e2e_full.c). It tests concurrent clients connected over live TCP sockets against the server.

### Run the Test Suite:
```bash
# Terminal 1: Start the Server
./server

# Terminal 2: Compile & Run the Tests
gcc -Wall -Wextra -pthread -O2 test_e2e_full.c crypto_utils.c e2ee.c file_transfer.c -o test_e2e_full -lssl -lcrypto
./test_e2e_full
```

### Verification Coverage:
1. **Double Ratchet E2EE**: Key directory discovery, multi-round message encryption/decryption, DH ratchet advances, forward secrecy.
2. **Chunked File Transfer**: Offer, interactive accept, Base64 chunk streaming, dynamic progress tracking, and byte-for-byte SHA-256 verification.
3. **Anti-Spam Token Bucket**: Burst traffic detection, exhaustion warnings, and automated 15-second mute enforcement.
4. **Offline Inbox**: Message queueing when recipient is disconnected, and automatic delivery replay upon reconnect.
5. **PBKDF2 Authentication**: Challenge-response handshake, dynamic password update (`/setadminpwd`), old password rejection, and new hash verification.

```text
============================================================
   SOC CHAT APP - FULL END-TO-END AUTOMATED TEST SUITE
============================================================
[TEST 1] Testing E2EE Double Ratchet Over Sockets...              PASSED ✓
[TEST 2] Testing Chunked File Transfer & SHA-256 Integrity...     PASSED ✓
[TEST 3] Testing Rate Limiting & Anti-Spam Defense...             PASSED ✓
[TEST 4] Testing Offline Messaging Inbox...                       PASSED ✓
[TEST 5] Testing Admin Authentication via PBKDF2...               PASSED ✓
============================================================
   ALL 5 END-TO-END TESTS COMPLETED AND PASSED (100%)!
============================================================
```

---

## 👨‍💻 Developer & License

Developed with passion by **[NEXUS8222](https://github.com/nexus8222)**  
*Systems Developer, Network Engineer & Cybersecurity Enthusiast.*

This project is licensed under the **MIT License**.
