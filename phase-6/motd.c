// motd.c
#include <stdio.h>
#include <string.h>
#include "motd.h"
#include "utils.h"

char motd[2048] = "Welcome to Phase 5 Chat Server!";

void load_motd() {
    FILE *fp = fopen("motd.txt", "r");
    if (fp) {
        fgets(motd, sizeof(motd), fp);
        fclose(fp);
    }
}

void set_motd(const char *new_msg) {
    strncpy(motd, new_msg, sizeof(motd) - 1);
    motd[sizeof(motd) - 1] = '\0';

    FILE *fp = fopen("motd.txt", "w");
    if (fp) {
        fprintf(fp, "%s\n", motd);
        fclose(fp);
    }
}

void send_motd(client_t *cli) {
    send_to_client(cli, "\033[1;36m[MOTD]\033[0m %s\n", motd);
}
