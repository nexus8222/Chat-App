#include "commands.h"
#include "log.h"
#include "banlist.h"
#include "mute.h"
#include "admin.h"
#include "motd.h"
#include "utils.h"
#include "client.h"

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
            "%s\n",
            cli->is_admin ?
                "/kick <user>\n/ban <user>\n/unban <user>\n/banlist\n"
                "/mute <user>\n/unmute <user>\n/mutelist\n"
                "/broadcast <msg>\n/log\n/shutdown\n/setmotd <msg>\n"
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

    if (strcmp(command, "users") == 0) {
        send_to_client(cli, "Connected users:\n");
        for (int i = 0; i < MAX_CLIENTS; ++i)
            if (clients[i])
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

    // === Admin Commands ===
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
