#include <stdio.h>
#include <string.h>
#include "common.h"
#include "utils.h"
#include "party.h"
#include "client.h"
#include <stdbool.h>
#define MAX_PARTIES 100
party_info_t party_list[MAX_PARTIES];
static char party_codes[MAX_PARTIES][PARTY_CODE_LEN];
int party_count = 0;

int party_exists(const char *code)
{
    for (int i = 0; i < party_count; ++i)
    {
        if (strcmp(party_codes[i], code) == 0)
            return 1;
    }
    return 0;
}

int create_party(const char *code)
{
    if (party_exists(code) || party_count >= MAX_PARTIES)
        return 0;

    strncpy(party_codes[party_count++], code, PARTY_CODE_LEN);
    return 1;
}

extern client_t *clients[MAX_CLIENTS];



extern client_t *clients[MAX_CLIENTS];



void list_active_parties(client_t *cli)
{
    char parties[BUFFER_SIZE];
    parties[0] = '\0';

   
    char seen_codes[MAX_PARTIES][PARTY_CODE_LEN];
    int seen_count = 0;

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i] && strlen(clients[i]->party_code) > 0)
        {
            bool already_listed = false;
            for (int k = 0; k < seen_count; ++k)
            {
                if (strcmp(seen_codes[k], clients[i]->party_code) == 0)
                {
                    already_listed = true;
                    break;
                }
            }

            if (!already_listed)
            {
                // Count members
                int count = 0;
                for (int j = 0; j < MAX_CLIENTS; ++j)
                    if (clients[j] && strcmp(clients[j]->party_code, clients[i]->party_code) == 0)
                        count++;

                // Append to output
                char line[128];
                snprintf(line, sizeof(line), " - %s (%d member%s)\n", clients[i]->party_code, count, count == 1 ? "" : "s");
                strncat(parties, line, sizeof(parties) - strlen(parties) - 1);

                // Mark this party as listed
                strncpy(seen_codes[seen_count++], clients[i]->party_code, PARTY_CODE_LEN);
            }
        }
    }

    if (strlen(parties) == 0)
        send_to_client(cli, "[SERVER] No active parties found.\n");
    else
        send_to_client(cli, "\033[1mActive Parties:\033[0m\n\n%s", parties);
}
