// commands.h
#ifndef COMMANDS_H
#define COMMANDS_H

#include "client.h"
#include "common.h"


int handle_command(char *command, client_t *cli);  // ← return type must match

#endif
