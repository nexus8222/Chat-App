// client_win_color.c
// Compile with: cl client_win_color.c /D_WIN32_WINNT=0x0601 /link ws2_32.lib
// Or: gcc client_win_color.c -o client.exe -lws2_32 (with MinGW)

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

// Thread for receiving messages
unsigned __stdcall recv_thread(void *arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int len = recv(sock, buffer, BUFFER_SIZE, 0);
        if (len <= 0) break;

        // Format incoming messages
        if (strstr(buffer, "[*]") == buffer || strstr(buffer, "**[SERVER]") == buffer)
            printf("\033[1;33m%s\033[0m", buffer); // Bold Yellow
        else if (strstr(buffer, "[PM from") == buffer)
            printf("\033[1;36m%s\033[0m", buffer); // Cyan
        else
            printf("%s", buffer);

        // Show prompt again after message
        printf("\033[1;32mYou> \033[0m");
        fflush(stdout);
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

    // Enable ANSI escape sequences in Windows console
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);

    // Setup socket
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

    // Username entry
    fgets(username, sizeof(username), stdin);
    send(sock, username, strlen(username), 0);

    // Launch receiving thread
    _beginthreadex(NULL, 0, recv_thread, NULL, 0, NULL);

    // Sending loop
    while (1) {
        printf("\033[1;32mYou> \033[0m");
        fflush(stdout);

        memset(buffer, 0, BUFFER_SIZE);
        if (!fgets(buffer, BUFFER_SIZE, stdin))
            break;

        send(sock, buffer, strlen(buffer), 0);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}
