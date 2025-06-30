// log.c
#include <stdio.h>
#include <time.h>
#include "log.h"
#include "common.h"
#include <string.h> 
void timestamp(char *buffer, size_t len) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, len, "%Y-%m-%d %H:%M:%S", t);
}

void log_connection(const char *username, const char *ip) {
    char ts[64];
    timestamp(ts, sizeof(ts));
    printf("\033[1;32m[%s] CONNECT: %s (%s)\033[0m\n", ts, username, ip);
}

void log_disconnection(const char *username, const char *ip) {
    char ts[64];
    timestamp(ts, sizeof(ts));
    printf("\033[1;31m[%s] DISCONNECT: %s (%s)\033[0m\n", ts, username, ip);
}

void log_server_start(int port) {
    char ts[64];
    timestamp(ts, sizeof(ts));
    printf("\033[1;34m[%s] SERVER STARTED on port %d\033[0m\n", ts, port);
}

void send_log_to_client(client_t *cli) {
    FILE *fp = fopen("server.log", "r");
    if (!fp) {
        send(cli->sockfd, "[SERVER] Log file not found.\n", 30, 0);
        return;
    }

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), fp)) {
        send(cli->sockfd, line, strlen(line), 0);
    }

    fclose(fp);
}
