/*

THIS IS CLIENTS C CODE FOR SENDING CONNECTIONS

*/

// This is same as phase one as it is doing our work fine we just modified the server.

// client.c
// gcc client.c -o client

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1" //HERE CHANGE THE IP ADDRESS OF THE SERVER
#define PORT 9001
#define BUFFER_SIZE 1024

int main() {
    int sock;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    //  Creating_socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[-] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Preparing server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("[-] Invalid address");
        close(sock);
        exit(EXIT_FAILURE);
    }

    // Connecting to the server
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[-] Connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("[+] Connected to server at %s:%d\n", SERVER_IP, PORT);
    printf("Type messages and press Enter. Ctrl+C to quit.\n");

    // Chat loop
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        if (!fgets(buffer, BUFFER_SIZE, stdin))
            break;

        send(sock, buffer, strlen(buffer), 0);
        ssize_t received = recv(sock, buffer, BUFFER_SIZE, 0);
        if (received <= 0) {
            printf("[-] Server closed connection\n");
            break;
        }

        printf("[Echo] %s", buffer);
    }

    //  Cleanup
    close(sock);
    return 0;
}