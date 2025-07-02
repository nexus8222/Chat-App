//commands.c
//handle all commands logics and sharing via cli.


#include "commands.h"
#include "log.h"
#include "banlist.h"
#include "mute.h"
#include "admin.h"
#include "motd.h"
#include "utils.h"
#include "client.h"
#include "party.h"
#include <stdint.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

extern client_t *clients[MAX_CLIENTS];
extern time_t server_start_time;

void broadcast_system(const char *msg) {
    char sysmsg[BUFFER_SIZE];
    snprintf(sysmsg, sizeof(sysmsg), "\033[1;36m[SERVER]: %s\033[0m\n", msg);

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i]) {
            send_to_client(clients[i], "%s", sysmsg);
        }
    }
}

int handle_command(const char *cmdline, client_t *cli) {
    if (!cmdline || cmdline[0] != '/') return 0;

    char command[64], arg1[64], arg2[BUFFER_SIZE];
    arg1[0] = arg2[0] = 0;
    sscanf(cmdline, "/%63s %63s %[^\n]", command, arg1, arg2);

    for (int i = 0; command[i]; i++) command[i] = tolower(command[i]);

    // === General Commands ===
    if (strcmp(command, "help") == 0) {
        send_to_client(cli,
            "\n\033[1mAvailable Commands:\033[0m\n"
            "/help\n/whoami\n/users\n/ping\n/time\n/uptime\n/motd\n"
            "/lastseen <user>\n"
            "/createparty \n/joinparty <code>\n/party\n/leaveparty\n"
            "%s\n",
            cli->is_admin ?
                "/kick <user>\n/ban <user>\n/unban <user>\n/banlist\n"
                "/mute <user>\n/unmute <user>\n/mutelist\n"
                "/broadcast <msg>\n/log\n/shutdown\n/setmotd <msg>\n/msg <user> <message>\n"
                : "");
        return 1;
    }

    if (strcmp(command, "whoami") == 0) {
        send_to_client(cli, "You are \033[1;32m%s\033[0m%s\n",
            cli->username, cli->is_admin ? " (admin)" : "");
        return 1;
    }

    if (strcmp(command, "ping") == 0) {
        send_to_client(cli, "pong\n");
        return 1;
    }
    
    if (strcmp(command, "listparties") == 0) {
    list_parties(cli);
    return 1;
    }

    if (strcmp(command, "users") == 0) {
    send_to_client(cli, "Connected users in your party:\n");
    for (int i = 0; i < MAX_CLIENTS; ++i)
        if (clients[i] && strcmp(clients[i]->party_code, cli->party_code) == 0)
            send_to_client(cli, " - %s%s\n", clients[i]->username,
                clients[i]->is_admin ? " (admin)" : "");
    return 1;
    }

    if (strcmp(command, "time") == 0) {
        time_t now = time(NULL);
        send_to_client(cli, "Server time: %s", ctime(&now));
        return 1;
    }

    if (strcmp(command, "uptime") == 0) {
        time_t now = time(NULL);
        int seconds = (int)(now - server_start_time);
        int hrs = seconds / 3600;
        int mins = (seconds % 3600) / 60;
        int secs = seconds % 60;
        send_to_client(cli, "Server uptime: %02d:%02d:%02d\n", hrs, mins, secs);
        return 1;
    }

    if (strcmp(command, "motd") == 0) {
        send_motd(cli);
        return 1;
    }

    if (strcmp(command, "setmotd") == 0 && cli->is_admin) {
        char fullmsg[BUFFER_SIZE];
        snprintf(fullmsg, sizeof(fullmsg), "%s %s", arg1, arg2);
        set_motd(fullmsg);
        broadcast_system("MOTD updated.");
        return 1;
    }

    if (strcmp(command, "lastseen") == 0) {
        send_to_client(cli, "Tracking not implemented yet.\n");
        return 1;
    }


    if (strcmp(command, "createparty") == 0) {
    char partycode[PARTY_CODE_LEN];

    if (strlen(arg1) == 0){
        const char *charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        srand(time(NULL) ^ (intptr_t)cli);
        for (int i = 0; i < 6; ++i)
            partycode[i] = charset[rand() % 36];
        partycode[6] = '\0';
    } else {
        strncpy(partycode, arg1, PARTY_CODE_LEN);
    }

    
    if (party_exists(partycode)) {
        send_to_client(cli, "\033[1;31m[SERVER] Party already exists.\033[0m\n");
    } else if (create_party(partycode)) {
        strncpy(cli->party_code, partycode, PARTY_CODE_LEN);
        send_to_client(cli, "\033[1;32m[SERVER] Party '%s' created. Share the code to invite friends.\033[0m\n", partycode);
    } else {
        send_to_client(cli, "\033[1;31m[SERVER] Failed to create party. Limit reached?\033[0m\n");
    }

    return 1;
}

    if (strcmp(command, "joinparty") == 0) {
        if (strlen(arg1) == 0) {
            send_to_client(cli, "\033[1;33mUsage:\033[0m /joinparty <code>\n");
            return 1;
        }

        if (party_exists(arg1)) {
            strncpy(cli->party_code, arg1, PARTY_CODE_LEN);
            log_event("Party change: %s -> [%s]", cli->username, cli->party_code);
            send_to_client(cli, "\033[1;34m[SERVER] Joined party '%s'\033[0m\n", arg1);

        } else {
            send_to_client(cli, "\033[1;31m[SERVER] Party '%s' does not exist.\033[0m\n", arg1);
        }
        return 1;
    }

    if (strcmp(command, "party") == 0) {
        send_to_client(cli, "You are in party: \033[1;36m%s\033[0m\n",
            strlen(cli->party_code) ? cli->party_code : "public");
        return 1;
    }

    if (strcmp(command, "leaveparty") == 0) {
    cli->party_code[0] = '\0';
    log_event("Party leave: %s left their party", cli->username);
    send_to_client(cli, "\033[1;34m[SERVER] You left your current party and joined public chat.\033[0m\n");
    return 1;
    }

    if (strcmp(command, "msg") == 0) {
    if (strlen(arg1) == 0 || strlen(arg2) == 0) {
        send_to_client(cli, "Usage: /msg <username> <message>\n");
        return 1;
    }

    client_t *target = NULL;
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] && strcmp(clients[i]->username, arg1) == 0) {
            target = clients[i];
            break;
        }
    }

    if (!target) {
        send_to_client(cli, "\033[1;31m[SERVER] User '%s' not found or offline.\033[0m\n", arg1);
        return 1;
    }

    // Send to target
    send_to_client(target, "\033[1;35m[PM from %s]:\033[0m %s\n", cli->username, arg2);

    // Echo back to sender
    send_to_client(cli, "\033[1;36m[PM to %s]:\033[0m %s\n", target->username, arg2);
    return 1;
}


    if (cli->is_admin) {
        if (strcmp(command, "kick") == 0)
            return kick_user(arg1, cli);

        if (strcmp(command, "ban") == 0)
            return ban_user(arg1, cli);

        if (strcmp(command, "unban") == 0)
            return unban_user(arg1, cli);

        if (strcmp(command, "banlist") == 0){
           list_banned(cli);
           return 0;
        }

        if (strcmp(command, "mute") == 0)
            return mute_user(arg1, cli);

        if (strcmp(command, "unmute") == 0)
            return unmute_user(arg1, cli);

        if (strcmp(command, "mutelist") == 0){
            list_muted(cli);
            return 0;
        }

        if (strcmp(command, "broadcast") == 0) {
            char fullmsg[BUFFER_SIZE];
            snprintf(fullmsg, sizeof(fullmsg), "%s %s", arg1, arg2);
            broadcast_system(arg1[0] ? fullmsg : "(empty)");
            return 1;
        }

        if (strcmp(command, "log") == 0){
            send_log_to_client(cli);
            return 0;
        }

        if (strcmp(command, "shutdown") == 0) {
            broadcast_system("Server is shutting down.");
            exit(0);
        }
    }

    send_to_client(cli, "\033[1;33mUnknown command:\033[0m %s\n", command);
    return 1;
}
