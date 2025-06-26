// client_win.c
// Compile with: cl client_win.c /D_WIN32_WINNT=0x0601 /link ws2_32.lib

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <process.h>

#pragma comment(lib, "ws2_32.lib")

#define BUFFER_SIZE 1024

SOCKET sock;
char username[32];

unsigned __stdcall recv_thread(void *arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int len = recv(sock, buffer, BUFFER_SIZE, 0);
        if (len <= 0) break;

        if (strstr(buffer, "[*]") == buffer || strstr(buffer, "**[SERVER]") == buffer)
            printf("[BOLD] %s", buffer); // Simulated styling
        else if (strstr(buffer, "[PM from") == buffer)
            printf("[PRIVATE] %s", buffer);
        else
            printf("%s", buffer);
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    struct sockaddr_in server_addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) {
        printf("[-] Socket failed\n");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("[-] Connection failed\n");
        return 1;
    }

    char buffer[BUFFER_SIZE];
    recv(sock, buffer, BUFFER_SIZE, 0);
    printf("%s", buffer);

    fgets(username, sizeof(username), stdin);
    send(sock, username, strlen(username), 0);

    _beginthreadex(NULL, 0, recv_thread, NULL, 0, NULL);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        if (!fgets(buffer, BUFFER_SIZE, stdin))
            break;
        send(sock, buffer, strlen(buffer), 0);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}

