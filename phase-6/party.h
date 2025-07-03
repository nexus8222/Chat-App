#ifndef PARTY_H
#define PARTY_H

int party_exists(const char *code);
int create_party(const char *code);
void list_parties(client_t *cli);

#endif
