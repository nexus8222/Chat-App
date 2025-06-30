// utils.c
#include <unistd.h>
#include <string.h>
#include "utils.h"
#include "common.h"

void send_to_client(client_t *cli, const char *msg) {
    write(cli->sockfd, msg, strlen(msg));
}
