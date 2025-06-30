// mute.h
#ifndef MUTE_H
#define MUTE_H
#include "common.h"

#include "client.h"

void mute_user(const char *username, client_t *requester);
void unmute_user(const char *username, client_t *requester);

#endif
