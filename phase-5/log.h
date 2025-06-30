// log.h
#ifndef LOG_H
#define LOG_H
#include "common.h"

void log_connection(const char *username, const char *ip);
void log_disconnection(const char *username, const char *ip);
void log_server_start(int port);

#endif
