#include "utils.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

void send_to_client(client_t *cli, const char *fmt, ...) {
    char buffer[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    send(cli->sockfd, buffer, strlen(buffer), 0);
}
