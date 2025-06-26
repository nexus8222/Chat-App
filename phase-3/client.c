/*
## CROSS-PLATFORM CLIENT WITH IP & PORT ARGUMENTS

Usage:
    ./client <SERVER_IP> <PORT>
    OR
    client.exe <SERVER_IP> <PORT>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #define _CRT_SECURE_NO_WARNINGS
    #include <winsock2.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef SOCKET socket_t;
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <pthread.h>
    #include <signal.h>
    typedef int socket_t;
#endif

#define BUFFER_SIZE 1024

socket_t sock;

#ifdef _WIN32
DWORD WINAPI receive_handler(LPVOID param) {
#else
void* receive_handler(void* param) {
#endif
    char buffer[BUFFER_SIZE];
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
#ifdef _WIN32
        int received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
#else
        ssize_t received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
#endif
        if (received <= 0) {
            printf("[-] Server disconnected\n");
#ifdef _WIN32
            closesocket(sock);
            WSACleanup();
#else
            close(sock);
#endif
            exit(0);
        }
        printf("%s", buffer);
        fflush(stdout);
    }
    return 0;
}

#ifndef _WIN32
void handle_sigint(int sig) {
    printf("\n[!] Client exiting...\n");
    close(sock);
    exit(0);
}
#endif

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <SERVER_IP> <PORT>\n", argv[0]);
        return 1;
    }

    const char* server_ip = argv[1];
    int port = atoi(argv[2]);

    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        fprintf(stderr, "[-] WSAStartup failed\n");
        return 1;
    }
#endif

    sock = socket(AF_INET, SOCK_STREAM, 0);
#ifdef _WIN32
    if (sock == INVALID_SOCKET) {
        fprintf(stderr, "[-] Socket creation failed\n");
        return 1;
    }
#else
    if (sock < 0) {
        perror("[-] Socket creation failed");
        return 1;
    }
    signal(SIGINT, handle_sigint);
#endif

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

#ifdef _WIN32
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        printf("[-] Invalid IP address\n");
        return 1;
    }
#else
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("[-] Invalid IP address");
        return 1;
    }
#endif

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
#ifdef _WIN32
        int err = WSAGetLastError();
        printf("[-] Connection failed. Error code: %d\n", err);
        closesocket(sock);
        WSACleanup();
#else
        perror("[-] Connection failed");
        close(sock);
#endif
        return 1;
    }

    printf("[+] Connected to server at %s:%d\n", server_ip, port);
    printf("[*] Type and press Enter to chat. Press Ctrl+C to quit.\n");

#ifdef _WIN32
    CreateThread(NULL, 0, receive_handler, NULL, 0, NULL);
#else
    pthread_t tid;
    pthread_create(&tid, NULL, receive_handler, NULL);
    pthread_detach(tid);
#endif

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        if (!fgets(buffer, BUFFER_SIZE, stdin))
            break;
        send(sock, buffer, strlen(buffer), 0);
    }

#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif

    return 0;
}

