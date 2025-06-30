// commands.c
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "commands.h"
#include "client.h"
#include "utils.h"
#include "admin.h"
#include "common.h"
extern time_t server_start_time;

int handle_command(char *cmd, client_t *cli) {
    char response[2048];

    if (strncmp(cmd, "/help", 5) == 0) {
        snprintf(response, sizeof(response),
            "\033[1mAvailable commands:\033[0m\n"
            "/help - Show this help\n"
            "/ping - Check latency\n"
            "/time - Server time\n"
            "/uptime - Server uptime\n"
            "/motd - Message of the day\n"
            "/kick <user>\n"
            "/ban <user|ip>\n"
            "/unban <ip>\n"
            "/mute <user>\n"
            "/unmute <user>\n"
        );
        send_to_client(cli, response);
        return 1;

    } else if (strncmp(cmd, "/ping", 5) == 0) {
        send_to_client(cli, "\033[1;32m[SERVER] pong\033[0m\n");
        return 1;

    } else if (strncmp(cmd, "/time", 5) == 0) {
        time_t now = time(NULL);
        struct tm *t = localtime(&now);
        strftime(response, sizeof(response), "\033[1;36m[SERVER] Time: %Y-%m-%d %H:%M:%S\033[0m\n", t);
        send_to_client(cli, response);
        return 1;

    } else if (strncmp(cmd, "/uptime", 7) == 0) {
        time_t now = time(NULL);
        int uptime = (int)(now - server_start_time);
        snprintf(response, sizeof(response), "\033[1;34m[SERVER] Uptime: %d seconds\033[0m\n", uptime);
        send_to_client(cli, response);
        return 1;

    } else if (strncmp(cmd, "/motd", 5) == 0) {
        FILE *fp = fopen("config/motd.txt", "r");
        if (fp) {
            while (fgets(response, sizeof(response), fp)) {
                send_to_client(cli, response);
            }
            fclose(fp);
        } else {
            send_to_client(cli, "\033[1;31m[SERVER] MOTD file not found.\033[0m\n");
        }
        return 1;
    }

    // If not matched, check if admin command
    return handle_admin_command(cmd, cli);
}
