// mute.c
#include <string.h>
#include "mute.h"
#include "client.h"
#include "utils.h"
#include "common.h"

extern client_t *clients[MAX_CLIENTS];

client_t* find_client_by_name(const char *name);  // Declare — defined in admin.c

void mute_user(const char *target, client_t *requester) {
    client_t *victim = find_client_by_name(target);
    if (!victim) {
        send_to_client(requester, "\033[1;33m[ADMIN] User not found.\033[0m\n");
        return;
    }

    victim->is_muted = 1;
    send_to_client(victim, "\033[1;33m[SERVER] You have been muted.\033[0m\n");
    send_to_client(requester, "\033[1;32m[ADMIN] User muted.\033[0m\n");
}

void unmute_user(const char *target, client_t *requester) {
    client_t *victim = find_client_by_name(target);
    if (!victim) {
        send_to_client(requester, "\033[1;33m[ADMIN] User not found.\033[0m\n");
        return;
    }

    victim->is_muted = 0;
    send_to_client(victim, "\033[1;32m[SERVER] You have been unmuted.\033[0m\n");
    send_to_client(requester, "\033[1;32m[ADMIN] User unmuted.\033[0m\n");
}
