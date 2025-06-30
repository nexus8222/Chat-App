// banlist.c
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h> 
#include "banlist.h"
#include "common.h"
#include "client.h" 
#define BAN_FILE "banned.txt"

static char banned_ips[MAX_BANNED][INET_ADDRSTRLEN];
static int banned_count = 0;

void str_trim_lf(char *s, int size) {
    for (int i = 0; i < size; i++) {
        if (s[i] == '\n') {
            s[i] = '\0';
            break;
        }
    }
}

void ban_ip(const char *ip) {
    for (int i = 0; i < banned_count; ++i) {
        if (strcmp(banned_ips[i], ip) == 0) return; // already banned
    }

    if (banned_count < MAX_BANNED) {
        strncpy(banned_ips[banned_count++], ip, INET_ADDRSTRLEN);
        FILE *fp = fopen(BAN_FILE, "a");
        if (fp) {
            fprintf(fp, "%s\n", ip);
            fclose(fp);
        }
    }
}

void unban_ip(const char *ip) {
    int found = 0;
    for (int i = 0; i < banned_count; ++i) {
        if (strcmp(banned_ips[i], ip) == 0) {
            found = 1;
            for (int j = i; j < banned_count - 1; ++j) {
                strncpy(banned_ips[j], banned_ips[j + 1], INET_ADDRSTRLEN);
            }
            banned_count--;
            break;
        }
    }

    if (found) {
        FILE *fp = fopen(BAN_FILE, "w");
        if (fp) {
            for (int i = 0; i < banned_count; ++i) {
                fprintf(fp, "%s\n", banned_ips[i]);
            }
            fclose(fp);
        }
    }
}

int is_ip_banned(const char *ip) {
    for (int i = 0; i < banned_count; ++i) {
        if (strcmp(banned_ips[i], ip) == 0) {
            return 1;
        }
    }
    return 0;
}

void load_banlist() {
    FILE *fp = fopen(BAN_FILE, "r");
    if (!fp) return;

    char line[INET_ADDRSTRLEN];
    while (fgets(line, sizeof(line), fp)) {
        str_trim_lf(line, sizeof(line));
        if (strlen(line) > 0 && banned_count < MAX_BANNED) {
            strncpy(banned_ips[banned_count++], line, INET_ADDRSTRLEN);
        }
    }

    fclose(fp);
}
 
void list_banned(client_t *cli) {
    char buffer[BUFFER_SIZE];
    snprintf(buffer, sizeof(buffer), "\033[1m[SERVER] Banned IPs:\033[0m\n");
    send(cli->sockfd, buffer, strlen(buffer), 0);

    if (banned_count == 0) {
        send(cli->sockfd, "None\n", 5, 0);
        return;
    }

    for (int i = 0; i < banned_count; ++i) {
        snprintf(buffer, sizeof(buffer), "%s\n", banned_ips[i]);
        send(cli->sockfd, buffer, strlen(buffer), 0);
    }
}
