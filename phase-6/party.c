#include <stdio.h>
#include <string.h>
#include "common.h"
#include "utils.h"

#define MAX_PARTIES 100

static char party_codes[MAX_PARTIES][PARTY_CODE_LEN];
static int party_count = 0;

int party_exists(const char *code) {
    for (int i = 0; i < party_count; ++i) {
        if (strcmp(party_codes[i], code) == 0)
            return 1;
    }
    return 0;
}

int create_party(const char *code) {
    if (party_exists(code) || party_count >= MAX_PARTIES)
        return 0;

    strncpy(party_codes[party_count++], code, PARTY_CODE_LEN);
    return 1;
}


extern client_t *clients[MAX_CLIENTS];

void list_parties(client_t *cli) {
    send_to_client(cli, "\033[1mActive Parties:\033[0m\n");
    for (int i = 0; i < party_count; ++i) {
        int members = 0;
        for (int j = 0; j < MAX_CLIENTS; ++j)
            if (clients[j] && strcmp(clients[j]->party_code, party_codes[i]) == 0)
                members++;

        send_to_client(cli, " - %s (%d member%s)\n",
            party_codes[i], members, members == 1 ? "" : "s");
    }
}
