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
// gcc server.c -o server   #For compiling.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 9001
#define BUFFER_SIZE 1024

int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // This is creating our sockets
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("[-] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    //  making server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // 0.0.0.0
    server_addr.sin_port = htons(PORT);

    //  Binding sockets
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[-] Bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    //  Listening for incoming connections from clients.
    if (listen(server_fd, 1) < 0) {
        perror("[-] Listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("[+] Server listening on port %d...\n", PORT);

    //  Accept client invitation
    client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        perror("[-] Accept failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("[+] Client connected from %s:%d\n",
        inet_ntoa(client_addr.sin_addr),
        ntohs(client_addr.sin_port));

    //  Communication loop
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            printf("[-] Client disconnected\n");
            break;
        }

        printf("[Client] %s", buffer);
        send(client_fd, buffer, bytes_received, 0);  // Echo
    }

    //  Cleanup of connections
    close(client_fd);
    close(server_fd);
    return 0;
}
