/*
##THIS ARE FEATURES

Multi-client support using pthreads

Client sends messages

Server broadcasts to all other clients

Clean disconnection handling

No memory leaks

Clear logging

*/
// server.c
// gcc server.c -o server -pthread

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 9001
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100

typedef struct {
    int socket;
    struct sockaddr_in address;
} Client;

Client clients[MAX_CLIENTS];
int client_count = 0;
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void broadcast(const char* message, int sender_fd) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; ++i) {
        if (clients[i].socket != sender_fd) {
            send(clients[i].socket, message, strlen(message), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void remove_client(int client_fd) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < client_count; ++i) {
        if (clients[i].socket == client_fd) {
            clients[i] = clients[client_count - 1]; // Replace with last
            client_count--;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void* handle_client(void* arg) {
    int client_fd = *(int*)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    getpeername(client_fd, (struct sockaddr*)&addr, &addr_len);

    printf("[+] Client connected: %s:%d\n",
        inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

    // Add to global client list
    pthread_mutex_lock(&clients_mutex);
    if (client_count < MAX_CLIENTS) {
        clients[client_count].socket = client_fd;
        clients[client_count].address = addr;
        client_count++;
    } else {
        fprintf(stderr, "[-] Max clients reached. Connection rejected.\n");
        close(client_fd);
        pthread_mutex_unlock(&clients_mutex);
        return NULL;
    }
    pthread_mutex_unlock(&clients_mutex);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf("[-] Client disconnected: %s:%d\n",
                inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
            break;
        }

        buffer[strcspn(buffer, "\n")] = 0; // Trim newline
        printf("[Client %s:%d] %s\n",
            inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), buffer);

        char message[BUFFER_SIZE + 64];
        snprintf(message, sizeof(message), "[%s:%d] %s\n",
            inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), buffer);

        broadcast(message, client_fd);
    }

    close(client_fd);
    remove_client(client_fd);
    return NULL;
}

int main() {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Creating socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("[-] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Server address setup
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Binding
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[-] Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listening
    if (listen(server_fd, 10) < 0) {
        perror("[-] Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("[+] Server listening on port %d...\n", PORT);

    while (1) {
        int* client_fd = malloc(sizeof(int));
        if (!client_fd) {
            perror("[-] Malloc failed");
            continue;
        }

        *client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (*client_fd < 0) {
            perror("[-] Accept failed");
            free(client_fd);
            continue;
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, client_fd) != 0) {
            perror("[-] Thread creation failed");
            close(*client_fd);
            free(client_fd);
            continue;
        }

        pthread_detach(tid);
    }

    close(server_fd);
    return 0;
}

