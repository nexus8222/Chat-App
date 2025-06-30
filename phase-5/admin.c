// admin.c
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h> 
#include "admin.h"
#include "client.h"
#include "banlist.h"
#include "mute.h"
#include "utils.h"
#include "common.h"

extern client_t *clients[MAX_CLIENTS];

client_t* find_client_by_name(const char *name) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] && strcmp(clients[i]->username, name) == 0) {
            return clients[i];
        }
    }
    return NULL;
}

void kick_user(const char *target, client_t *requester) {
    client_t *victim = find_client_by_name(target);
    if (!victim) {
        send_to_client(requester, "\033[1;33m[ADMIN] User not found.\033[0m\n");
        return;
    }

    send_to_client(victim, "\033[1;31m[SERVER] You have been kicked by admin.\033[0m\n");
    close(victim->sockfd);
}

void ban_user(const char *target, client_t *requester) {
    client_t *victim = find_client_by_name(target);
    if (victim) {
        ban_ip(victim->ip);

        send_to_client(victim, "\033[1;31m[SERVER] You are banned.\033[0m\n");
        close(victim->sockfd);
    } else {
        ban_ip(target); // fallback — allow banning by IP
    }

    send_to_client(requester, "\033[1;33m[ADMIN] Ban applied.\033[0m\n");
}

void unban_user(const char *ip, client_t *requester) {
    unban_ip(ip);
    send_to_client(requester, "\033[1;32m[ADMIN] Unban applied.\033[0m\n");
}

int handle_admin_command(char *cmd, client_t *cli) {
    if (!cli->is_admin) {
        send_to_client(cli, "\033[1;31m[SERVER] Unauthorized command.\033[0m\n");
        return 1;
    }

    char arg[64];

    if (sscanf(cmd, "/kick %63s", arg) == 1) {
        kick_user(arg, cli);
        return 1;
    } else if (sscanf(cmd, "/ban %63s", arg) == 1) {
        ban_user(arg, cli);
        return 1;
    } else if (sscanf(cmd, "/unban %63s", arg) == 1) {
        unban_user(arg, cli);
        return 1;
    } else if (sscanf(cmd, "/mute %63s", arg) == 1) {
        mute_user(arg, cli);     // <- comes from mute.c now
        return 1;
    } else if (sscanf(cmd, "/unmute %63s", arg) == 1) {
        unmute_user(arg, cli);   // <- comes from mute.c now
        return 1;
    }

    return 0; // Not matched — let handle_command() continue
}
