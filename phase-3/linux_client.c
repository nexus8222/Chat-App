/*
##THIS IS CLIENTS C CODE FOR SENDING CONNECTIONS

Multi-threaded client for real-time chatroom

Supports:
- Sending user input to server
- Receiving broadcasts from server asynchronously
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT 9001
#define BUFFER_SIZE 1024

int sock;

void* receive_handler(void* arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (received <= 0) {
            printf("[-] Disconnected from server\n");
            exit(0);
        }
        printf("%s", buffer);
        fflush(stdout);
    }
    return NULL;
}

void handle_sigint(int sig) {
    printf("\n[!] Client exiting...\n");
    close(sock);
    exit(0);
}

int main() {
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    signal(SIGINT, handle_sigint); // Ctrl+C handler

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[-] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("[-] Invalid address");
        close(sock);
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[-] Connection failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("[+] Connected to chat server at %s:%d\n", SERVER_IP, PORT);
    printf("[*] Type and press Enter to chat. Press Ctrl+C to exit.\n");

    pthread_t recv_thread;
    if (pthread_create(&recv_thread, NULL, receive_handler, NULL) != 0) {
        perror("[-] Failed to create receive thread");
        close(sock);
        exit(EXIT_FAILURE);
    }

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        if (!fgets(buffer, BUFFER_SIZE, stdin)) {
            break;
        }
        send(sock, buffer, strlen(buffer), 0);
    }

    close(sock);
    return 0;
}

