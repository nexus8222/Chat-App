// common.h
#ifndef COMMON_H
#define COMMON_H

#include <time.h>
#include <netinet/in.h>  // for struct sockaddr_in
#include "common.h"

#define USERNAME_LEN 32
#define BUFFER_SIZE 2048
#define MAX_CLIENTS 100

typedef struct client {
    int sockfd;
    char username[USERNAME_LEN];
    char ip[INET_ADDRSTRLEN];   // use system-defined 16
    int is_admin;
    int is_muted;
    time_t connected_time;
    struct sockaddr_in address;  // used by inet_ntop etc.
    time_t join_time;
    time_t last_seen;
    struct client *next;
} client_t;

extern client_t *clients[MAX_CLIENTS];

#endif
