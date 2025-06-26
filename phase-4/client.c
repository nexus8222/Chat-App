// client.c
// gcc client.c -o client

/*


  # FEATURES IMPLEMENTED:

  - Connects to a remote chat server using TCP
  - Prompts the user for a username on startup
  - Sends messages typed from stdin to the server
  - Receives and prints:
      • Public messages from all users
      • Private messages sent via @username syntax
      • Bold/pinned server messages (visually styled)
  - Automatically colored or tagged output:
      • [PM from user] shown in cyan
      • [*] join/leave notices in yellow
      • **[SERVER]** styled as bold/pinned
  - Handles server disconnection gracefully

  # @MENTIONS:
    Sending @bob Hello will send a private message to `bob` only.


  
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

int sock;
char username[32];

void *recv_thread(void *arg) {
    char buffer[BUFFER_SIZE];
    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        int len = recv(sock, buffer, BUFFER_SIZE, 0);
        if (len <= 0) break;

        // Check if server message
        if (strstr(buffer, "[*]") == buffer || strstr(buffer, "**[SERVER]") == buffer)
            printf("\033[1;33m%s\033[0m", buffer); // bold yellow
        else if (strstr(buffer, "[PM from") == buffer)
            printf("\033[1;36m%s\033[0m", buffer); // cyan private
        else
            printf("%s", buffer);
    }
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: %s <server_ip> <port>\n", argv[0]);
        return 1;
    }

    struct sockaddr_in server_addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[-] Socket failed");
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(argv[2]));
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        perror("[-] Invalid address");
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("[-] Connection failed");
        return 1;
    }

    char buffer[BUFFER_SIZE];
    recv(sock, buffer, BUFFER_SIZE, 0);
    printf("%s", buffer); // prompt

    fgets(username, sizeof(username), stdin);
    send(sock, username, strlen(username), 0);

    pthread_t tid;
    pthread_create(&tid, NULL, recv_thread, NULL);

    while (1) {
        memset(buffer, 0, BUFFER_SIZE);
        if (!fgets(buffer, BUFFER_SIZE, stdin))
            break;
        send(sock, buffer, strlen(buffer), 0);
    }

    close(sock);
    return 0;
}

