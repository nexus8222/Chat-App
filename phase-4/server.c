// Phase 4 Chat App - Server
// gcc server.c -o server -lpthread

/*

  # FEATURES IMPLEMENTED:

  - Accepts multiple clients via pthreads
  - Each client is prompted for a unique username
  - Broadcasts join and leave notifications to all clients
  - Supports public messages (e.g., [alice]: Hello everyone)
  - Supports private @mentions (e.g., @bob Hello -> only bob sees it)
  - Server messages appear as bold/pinned (e.g., **[SERVER]: Notice**)
  - Handles graceful client disconnections
  - Prevents buffer overflow (static buffer size used)
  - Thread-safe client list using mutex
  - Rejects new connections when server is full

*/

// Phase 4 Chat App - Server with Server Messaging
// gcc server.c -o server -lpthread

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int socket;
    char username[32];
    pthread_t thread;
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void broadcast(char *msg, int exclude_sock) {
    pthread_mutex_lock(&lock);
    for (int i = 0; i < client_count; ++i) {
        if (clients[i].socket != exclude_sock) {
            send(clients[i].socket, msg, strlen(msg), 0);
        }
    }
    pthread_mutex_unlock(&lock);
}

Client* find_client_by_name(const char *name) {
    for (int i = 0; i < client_count; ++i) {
        if (strcmp(clients[i].username, name) == 0) {
            return &clients[i];
        }
    }
    return NULL;
}

void *handle_client(void *arg) {
    Client *cli = (Client *)arg;
    char buffer[BUFFER_SIZE];
    char msg[BUFFER_SIZE + 64];

    // Ask for unique username
    while (1) {
        send(cli->socket, "Enter username: ", 17, 0);
        int len = recv(cli->socket, buffer, BUFFER_SIZE, 0);
        if (len <= 0) {
            close(cli->socket);
            return NULL;
        }
        buffer[strcspn(buffer, "\n")] = 0;

        pthread_mutex_lock(&lock);
        int name_taken = find_client_by_name(buffer) != NULL;
        pthread_mutex_unlock(&lock);

        if (name_taken) {
            char *taken = "[!] Username taken. Try another.\n";
            send(cli->socket, taken, strlen(taken), 0);
        } else {
            strncpy(cli->username, buffer, sizeof(cli->username));
            break;
        }
    }

    snprintf(msg, sizeof(msg), "[*] %s joined the chat.\n", cli->username);
    printf("%s", msg);
    fflush(stdout);
    broadcast(msg, -1);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes = recv(cli->socket, buffer, BUFFER_SIZE, 0);
        if (bytes <= 0)
            break;

        buffer[strcspn(buffer, "\n")] = 0;

        // Check for @mention
        if (buffer[0] == '@') {
            char *target_name = strtok(buffer + 1, " ");
            char *private_msg = strtok(NULL, "\0");
            if (target_name && private_msg) {
                Client *target = find_client_by_name(target_name);
                if (target) {
                    snprintf(msg, sizeof(msg), "[PM from %s]: %s\n", cli->username, private_msg);
                    send(target->socket, msg, strlen(msg), 0);
                }
            }
            continue;
        }

        // Check for server pinned message
        if (strncmp(buffer, "**", 2) == 0) {
            snprintf(msg, sizeof(msg), "**[SERVER]: %s\n", buffer + 2);
            printf("%s", msg);
            fflush(stdout);
            broadcast(msg, -1);
            continue;
        }

        // Broadcast to all
        snprintf(msg, sizeof(msg), "[%s]: %s\n", cli->username, buffer);
        printf("%s", msg);
        fflush(stdout);
        broadcast(msg, cli->socket);
    }

    // Client disconnected
    snprintf(msg, sizeof(msg), "[*] %s left the chat.\n", cli->username);
    printf("%s", msg);
    fflush(stdout);
    broadcast(msg, cli->socket);

    pthread_mutex_lock(&lock);
    for (int i = 0; i < client_count; ++i) {
        if (clients[i].socket == cli->socket) {
            for (int j = i; j < client_count - 1; ++j)
                clients[j] = clients[j + 1];
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&lock);

    close(cli->socket);
    return NULL;
}

void *server_input_thread(void *arg) {
    char input[BUFFER_SIZE];
    char msg[BUFFER_SIZE + 64];

    while (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = 0;

        if (strlen(input) == 0)
            continue;

        snprintf(msg, sizeof(msg), "**[SERVER]: %s\n", input);
        printf("%s", msg);
        fflush(stdout);
        broadcast(msg, -1);
    }
    return NULL;
}

int main() {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("[-] Socket failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(9001);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[-] Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("[-] Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("[+] Server listening on port 9001...\n");
    fflush(stdout);

    pthread_t server_input_tid;
    pthread_create(&server_input_tid, NULL, server_input_thread, NULL);

    while (1) {
        int new_sock = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (new_sock < 0) continue;

        pthread_mutex_lock(&lock);
        if (client_count >= MAX_CLIENTS) {
            char *full = "[!] Server full. Try again later.\n";
            send(new_sock, full, strlen(full), 0);
            close(new_sock);
            pthread_mutex_unlock(&lock);
            continue;
        }

        clients[client_count].socket = new_sock;
        pthread_create(&clients[client_count].thread, NULL, handle_client, &clients[client_count]);
        client_count++;
        pthread_mutex_unlock(&lock);
    }

    close(server_fd);
    return 0;
}