#include "utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

extern client_t *clients[MAX_CLIENTS]; 
extern pthread_mutex_t clients_mutex;

void send_to_client(client_t *cli, const char *fmt, ...) {
    char buffer[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    send(cli->sockfd, buffer, strlen(buffer), 0);
}


void broadcast_message(const char *msg, client_t *sender) {
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] && 
            strcmp(clients[i]->party_code, sender->party_code) == 0){
            send(clients[i]->sockfd, msg, strlen(msg), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}