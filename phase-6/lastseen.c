#include "lastseen.h"
#include <stdio.h>
#include <string.h>

lastseen_t lastseen_list[MAX_CLIENTS];

void update_lastseen(const char *username) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (strcmp(lastseen_list[i].username, username) == 0) {
            lastseen_list[i].timestamp = time(NULL);
            return;
        }
        if (strlen(lastseen_list[i].username) == 0) {
            strncpy(lastseen_list[i].username, username, sizeof(lastseen_list[i].username));
            lastseen_list[i].timestamp = time(NULL);
            return;
        }
    }
}

void save_lastseen_to_file() {
    FILE *fp = fopen(".lastseen", "wb");
    if (!fp) return;
    fwrite(lastseen_list, sizeof(lastseen_t), MAX_CLIENTS, fp);
    fclose(fp);
}

void load_lastseen_from_file() {
    FILE *fp = fopen(".lastseen", "rb");
    if (!fp) return;
    fread(lastseen_list, sizeof(lastseen_t), MAX_CLIENTS, fp);
    fclose(fp);
}
