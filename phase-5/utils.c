#include "utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

extern client_t *clients[MAX_CLIENTS]; 

void send_to_client(client_t *cli, const char *fmt, ...) {
    char buffer[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    send(cli->sockfd, buffer, strlen(buffer), 0);
}

void broadcast_message(const char *msg, client_t *sender) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        client_t *cli = clients[i];
        if (cli && cli != sender) {
            if (strcmp(cli->party_code, sender->party_code) == 0) {
                send_to_client(cli, "[%s]: %s\n", sender->username, msg);
            }
        }
    }
}
