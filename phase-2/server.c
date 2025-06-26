/*
##THIS ARE FEATURES 

Single client connects to server

Client sends messages

Server echoes back messages

Clean disconnection handling

No memory leaks

Clear logging

*/
// server.c
// gcc server.c -o server -pthread   #For compiling.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 9001
#define BUFFER_SIZE 1024

// Thread handler for each client
void* handle_client(void* arg) {
    int client_fd = *(int*)arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    getpeername(client_fd, (struct sockaddr*)&addr, &len);

    printf("[+] Client connected from %s:%d\n",
        inet_ntoa(addr.sin_addr),
        ntohs(addr.sin_port));

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf("[-] Client %s:%d disconnected\n",
                inet_ntoa(addr.sin_addr),
                ntohs(addr.sin_port));
            break;
        }

        printf("[Client %s:%d] %s",
            inet_ntoa(addr.sin_addr),
            ntohs(addr.sin_port),
            buffer);

        send(client_fd, buffer, bytes_received, 0);
    }

    close(client_fd);
    return NULL;
}

int main() {
    int server_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // This is creating our sockets
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("[-] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // making server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
    server_addr.sin_port = htons(PORT);

    // Binding sockets
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[-] Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listening for incoming connections from clients.
    if (listen(server_fd, 10) < 0) {
        perror("[-] Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("[+] Server listening on port %d...\n", PORT);

    // Accept loop
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

        pthread_detach(tid); // No need to join
    }

    close(server_fd);
    return 0;
}
