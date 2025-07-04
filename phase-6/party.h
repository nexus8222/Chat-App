#ifndef PARTY_H
#define PARTY_H
#define MAX_PARTIES 100
int party_exists(const char *code);
int create_party(const char *code);
void list_active_parties(client_t *cli);
typedef struct {
    char code[PARTY_CODE_LEN];
    int is_locked;
    char invited_users[MAX_CLIENTS][USERNAME_LEN];
    int invite_count;
} party_info_t;

extern party_info_t party_list[MAX_PARTIES];

extern party_info_t party_list[MAX_PARTIES];
extern int party_count;

#endif
