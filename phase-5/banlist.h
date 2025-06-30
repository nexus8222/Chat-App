// banlist.h
#ifndef BANLIST_H
#define BANLIST_H

#include <netinet/in.h>
#include "common.h"

#define MAX_BANNED 100


void ban_ip(const char *ip);
void unban_ip(const char *ip);
int is_ip_banned(const char *ip);
void load_banlist(void);

#endif
