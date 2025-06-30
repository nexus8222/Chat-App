// log.c
#include <stdio.h>
#include <time.h>
#include "log.h"
#include "common.h"

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
