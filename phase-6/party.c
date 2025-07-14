// party.c
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "common.h"
#include "utils.h"
#include "party.h"
#include "client.h"

party_info_t party_list[MAX_PARTIES];
int party_count = 0;
extern client_t *clients[MAX_CLIENTS];

int party_exists(const char *code)
{
    for (int i = 0; i < party_count; ++i)
    {
        if (strcmp(party_list[i].code, code) == 0)
        {
            return 1;
        }
    }
    return 0;
}

int create_party(const char *code)
{
    if (party_exists(code) || party_count >= MAX_PARTIES)
        return 0;

    strncpy(party_list[party_count].code, code, PARTY_CODE_LEN);
    party_list[party_count].code[PARTY_CODE_LEN - 1] = '\0'; // safety
    party_list[party_count].is_locked = 0;
    party_list[party_count].invite_count = 0;
    party_count++;
    return 1;
}

void list_active_parties(client_t *cli)
{
    char parties[BUFFER_SIZE];
    parties[0] = '\0';

    for (int i = 0; i < party_count; ++i)
    {
        const char *code = party_list[i].code;
        int count = 0;

        for (int j = 0; j < MAX_CLIENTS; ++j)
        {
            if (clients[j] && strcmp(clients[j]->party_code, code) == 0)
            {
                count++;
            }
        }

        if (count > 0)
        {
            char line[128];
            snprintf(line, sizeof(line), " - %.40s (%d member%s)%s\n",
                     code, count, count == 1 ? "" : "s",
                     party_list[i].is_locked ? " [locked]" : "");

            size_t space_left = sizeof(parties) - strlen(parties) - 1;
            strncat(parties, line, space_left);
        }
    }

    if (strlen(parties) == 0)
        send_to_client(cli, "[SERVER] No active parties found.\n");
    else
        send_to_client(cli, "\033[1mActive Parties:\033[0m\n\n%s", parties);
}

void party_broadcast(const char *msg, const char *party_code, int except_sock) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] && strcmp(clients[i]->party_code, party_code) == 0 && clients[i]->sockfd != except_sock) {
            send(clients[i]->sockfd, msg, strlen(msg), 0);
        }
    }
}
