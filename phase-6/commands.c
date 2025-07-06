// commands.c
// Handle all commands: logic and sharing via CLI

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
#include "lastseen.h"
extern client_t *clients[MAX_CLIENTS];
extern time_t server_start_time;
extern lastseen_t lastseen_list[MAX_CLIENTS];
extern char pinned_message[BUFFER_SIZE];

void broadcast_system(const char *msg)
{
    char sysmsg[BUFFER_SIZE];
    snprintf(sysmsg, sizeof(sysmsg), "\033[1;36m[SERVER]: %s\033[0m\n", msg);
    for (int i = 0; i < MAX_CLIENTS; ++i)
        if (clients[i])
            send_to_client(clients[i], "%s", sysmsg);
}

int handle_command(const char *cmdline, client_t *cli)
{
    if (!cmdline || cmdline[0] != '/')
        return 0;

    char command[64], arg1[64], arg2[BUFFER_SIZE];
    arg1[0] = arg2[0] = 0;
    sscanf(cmdline, "/%63s %63s %[^\n]", command, arg1, arg2);
    for (int i = 0; command[i]; i++)
        command[i] = tolower(command[i]);

    // === General Commands ===
    if (strcmp(command, "help") == 0)
    {
        send_to_client(cli,
                       "\n\033[1mAvailable Commands:\033[0m\n"
                       "/help\n/whoami\n/users\n/ping\n/time\n/uptime\n/motd\n"
                       "/lastseen <user>\n"
                       "/createparty [code]\n/joinparty <code>\n/party\n/leaveparty\n"
                       "/msg <user> <message>\n"
                       "/editlast <msg>\n"
                       "/deletelast \n/setcolor <color>\n/colorlist\n"
                       "%s\n",
                       cli->is_admin ? "/kick <user>\n/ban <user>\n/unban <user>\n/banlist\n"
                                       "/mute <user>\n/unmute <user>\n/mutelist\n"
                                       "/broadcast <msg>\n/log\n/shutdown\n/setmotd <msg>\n/parties\n"
                                       "/lockparty\n/invite <user>\n/partyinfo <code>\n "

                                     : "");
        return 1;
    }

    if (strcmp(command, "lockparty") == 0 && cli->is_admin)
    {
        if (strlen(cli->party_code) == 0)
        {
            send_to_client(cli, "[SERVER] You are not in a party.\n");
            return 1;
        }

        for (int i = 0; i < party_count; ++i)
        {
            if (strcmp(party_list[i].code, cli->party_code) == 0)
            {
                party_list[i].is_locked = 1;
                send_to_client(cli, "[SERVER] Party '%s' is now locked.\n", cli->party_code);
                return 1;
            }
        }

        send_to_client(cli, "[SERVER] Failed to find your party.\n");
        return 1;
    }

    if (strcmp(command, "invite") == 0 && cli->is_admin)
    {
        if (strlen(cli->party_code) == 0)
        {
            send_to_client(cli, "[SERVER] You're not in a party.\n");
            return 1;
        }

        client_t *target = NULL;
        for (int i = 0; i < MAX_CLIENTS; ++i)
        {
            if (clients[i] && strcmp(clients[i]->username, arg1) == 0)
            {
                target = clients[i];
                break;
            }
        }

        if (!target)
        {
            send_to_client(cli, "[SERVER] User '%s' not found.\n", arg1);
            return 1;
        }

        strncpy(target->invited_party, cli->party_code, PARTY_CODE_LEN);
        send_to_client(target, "[SERVER] You have been invited to party '%s' by %s.\n", cli->party_code, cli->username);
        send_to_client(cli, "[SERVER] Invitation sent to %s.\n", target->username);
        return 1;
    }

    if (strcmp(command, "partyinfo") == 0 && cli->is_admin)
    {
        if (strlen(arg1) == 0)
        {
            send_to_client(cli, "[SERVER] Usage: /partyinfo <code>\n");
            return 1;
        }

        int found = 0;
        send_to_client(cli, "\033[1mMembers in party '%s':\033[0m\n", arg1);
        for (int i = 0; i < MAX_CLIENTS; ++i)
        {
            if (clients[i] && strcmp(clients[i]->party_code, arg1) == 0)
            {
                send_to_client(cli, " - %s%s\n", clients[i]->username,
                               clients[i]->is_admin ? " (admin)" : "");
                found = 1;
            }
        }

        if (!found)
        {
            send_to_client(cli, "[SERVER] No members found in party '%s'.\n", arg1);
        }

        return 1;
    }

    if (strcmp(command, "whoami") == 0)
    {
        send_to_client(cli, "You are \033[1;32m%s\033[0m%s\n",
                       cli->username, cli->is_admin ? " (admin)" : "");
        return 1;
    }

    if (strcmp(command, "ping") == 0)
    {
        send_to_client(cli, "pong\n");
        return 1;
    }

    if (strcmp(command, "editlast") == 0)
    {
        if (difftime(time(NULL), cli->last_msg_time) > 30)
        {
            send_to_client(cli, "[SERVER] Edit time window (30s) expired.\n");
        }
        else if (strlen(arg1) == 0)
        {
            send_to_client(cli, "[SERVER] Usage: /editlast <new message>\n");
        }
        else
        {
            char newmsg[BUFFER_SIZE];
            snprintf(newmsg, sizeof(newmsg), "%s %s", arg1, arg2);

            char edited[BUFFER_SIZE * 2];
            snprintf(edited, sizeof(edited), "\033[1;33m[EDITED] %s: %s\033[0m\n", cli->username, newmsg);
            broadcast_message(edited, cli);

            strncpy(cli->last_msg, newmsg, BUFFER_SIZE);
            cli->last_msg_time = time(NULL);

            log_connection("[EDIT] %s edited their last message.", cli->username);
        }
        return 1;
    }

    if (strcmp(command, "deletelast") == 0)
    {
        if (difftime(time(NULL), cli->last_msg_time) > 30)
        {
            send_to_client(cli, "[SERVER] Delete time window (30s) expired.\n");
        }
        else
        {
            char notice[BUFFER_SIZE];
            snprintf(notice, sizeof(notice), "\033[1;31m[DELETED] %s deleted their last message.\033[0m\n", cli->username);
            broadcast_message(notice, cli);

            cli->last_msg[0] = '\0';
            cli->last_msg_time = 0;

            log_connection("[DELETE] %s deleted their last message.", cli->username);
        }
        return 1;
    }

    if (strcmp(command, "users") == 0)
    {
        send_to_client(cli, "Connected users in your party:\n");
        for (int i = 0; i < MAX_CLIENTS; ++i)
            if (clients[i] && strcmp(clients[i]->party_code, cli->party_code) == 0)
                send_to_client(cli, " - %s%s\n", clients[i]->username,
                               clients[i]->is_admin ? " (admin)" : "");
        return 1;
    }

    if (strcmp(command, "time") == 0)
    {
        time_t now = time(NULL);
        send_to_client(cli, "Server time: %s", ctime(&now));
        return 1;
    }

    if (strcmp(command, "uptime") == 0)
    {
        time_t now = time(NULL);
        int seconds = (int)(now - server_start_time);
        send_to_client(cli, "Server uptime: %02d:%02d:%02d\n",
                       seconds / 3600, (seconds / 60) % 60, seconds % 60);
        return 1;
    }

    if (strcmp(command, "motd") == 0)
    {
        send_motd(cli);
        return 1;
    }

    if (strcmp(command, "setmotd") == 0 && cli->is_admin)
    {
        char fullmsg[BUFFER_SIZE];
        snprintf(fullmsg, sizeof(fullmsg), "%s %s", arg1, arg2);
        set_motd(fullmsg);
        broadcast_system("MOTD updated.");
        return 1;
    }

#define COLOR_RESET "\033[0m"

    if (strcmp(command, "setcolor") == 0)
    {
        if (strlen(arg1) == 0)
        {
            send_to_client(cli, "[USAGE] /setcolor <color>\n");
            return 1;
        }

        const char *color_code = NULL;

        if (strcasecmp(arg1, "red") == 0)
            color_code = "\033[0;31m";
        else if (strcasecmp(arg1, "green") == 0)
            color_code = "\033[0;32m";
        else if (strcasecmp(arg1, "yellow") == 0)
            color_code = "\033[0;33m";
        else if (strcasecmp(arg1, "blue") == 0)
            color_code = "\033[0;34m";
        else if (strcasecmp(arg1, "magenta") == 0)
            color_code = "\033[0;35m";
        else if (strcasecmp(arg1, "cyan") == 0)
            color_code = "\033[0;36m";
        else if (strcasecmp(arg1, "white") == 0)
            color_code = "\033[0;37m";
        else
        {
            send_to_client(cli, "[ERROR] Unsupported color.\nUse /colorlist to view available options.\n");
            return 1;
        }

        strncpy(cli->color, color_code, sizeof(cli->color));
        cli->color[sizeof(cli->color) - 1] = '\0';

        char msg[BUFFER_SIZE];
        snprintf(msg, sizeof(msg), "[SERVER] Username color set to %s%s%s\n", cli->color, arg1, COLOR_RESET);
        send_to_client(cli, msg);
        return 1;
    }

    if (strcmp(command, "colorlist") == 0)
    {
        send_to_client(cli,
                       "[SERVER] Available colors:\n"
                       "\033[0;31mred\033[0m, "
                       "\033[0;32mgreen\033[0m, "
                       "\033[0;33myellow\033[0m, "
                       "\033[0;34mblue\033[0m, "
                       "\033[0;35mmagenta\033[0m, "
                       "\033[0;36mcyan\033[0m, "
                       "\033[0;37mwhite\033[0m\n");
        return 1;
    }

    if (strcmp(command, "lastseen") == 0)
    {
        if (strlen(arg1) == 0)
        {
            send_to_client(cli, "[SERVER] Usage: /lastseen <user>\n");
            return 1;
        }

        for (int i = 0; i < MAX_CLIENTS; ++i)
        {
            if (clients[i] && strcmp(clients[i]->username, arg1) == 0)
            {
                send_to_client(cli, "[SERVER] User is currently online.\n");
                return 1;
            }
        }

        for (int i = 0; i < MAX_CLIENTS; ++i)
        {
            if (strlen(lastseen_list[i].username) > 0 &&
                strcmp(lastseen_list[i].username, arg1) == 0)
            {
                char buff[128];
                struct tm *tm_info = localtime(&lastseen_list[i].timestamp);
                strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", tm_info);
                send_to_client(cli, "[SERVER] %s was last seen at %s\n", arg1, buff);
                return 1;
            }
        }

        send_to_client(cli, "[SERVER] No record for that user.\n");
        return 1;
    }

    if (strcmp(command, "createparty") == 0)
    {
        char partycode[PARTY_CODE_LEN];
        if (strlen(arg1) == 0)
        {
            const char *charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
            srand(time(NULL) ^ (intptr_t)cli);
            for (int i = 0; i < 6; ++i)
                partycode[i] = charset[rand() % 36];
            partycode[6] = '\0';
        }
        else
        {
            strncpy(partycode, arg1, PARTY_CODE_LEN);
        }

        if (party_exists(partycode))
        {
            send_to_client(cli, "\033[1;31m[SERVER] Party already exists.\033[0m\n");
        }
        else if (create_party(partycode))
        {
            strncpy(cli->party_code, partycode, PARTY_CODE_LEN);
            send_to_client(cli, "\033[1;32m[SERVER] Party '%s' created.\033[0m\n", partycode);
        }
        else
        {
            send_to_client(cli, "\033[1;31m[SERVER] Failed to create party.\033[0m\n");
        }
        return 1;
    }

    if (strcmp(command, "joinparty") == 0)
    {
        if (strlen(arg1) == 0)
        {
            send_to_client(cli, "[SERVER] Usage: /joinparty <code>\n");
            return 1;
        }

        if (party_exists(arg1))
        {

            for (int i = 0; i < party_count; ++i)
            {
                if (strcmp(party_list[i].code, arg1) == 0)
                {
                    if (party_list[i].is_locked && strcmp(cli->invited_party, arg1) != 0)
                    {
                        send_to_client(cli, "[SERVER] Party '%s' is locked. You need an invite.\n", arg1);
                        return 1;
                    }
                }
            }

            strncpy(cli->party_code, arg1, PARTY_CODE_LEN);
            log_event("Party join: %s -> [%s]", cli->username, arg1);
            send_to_client(cli, "\033[1;34m[SERVER] Joined party '%s'\033[0m\n", arg1);
        }
        else
        {
            send_to_client(cli, "\033[1;31m[SERVER] Party '%s' does not exist.\033[0m\n", arg1);
        }
        return 1;
    }

    if (strcmp(command, "party") == 0)
    {
        if (strlen(cli->party_code) > 0)
        {
            send_to_client(cli, "[SERVER] You are in party: %s\n", cli->party_code);
        }
        else
        {
            send_to_client(cli, "[SERVER] You are not in a party.\n");
        }
        return 1;
    }

    if (strcmp(command, "lockparty") == 0 && cli->is_admin)
    {
        if (strlen(cli->party_code) == 0)
        {
            send_to_client(cli, "[SERVER] You are not in a party.\n");
            return 1;
        }

        int found = 0;
        for (int i = 0; i < party_count; i++)
        {
            if (strcmp(party_list[i].code, cli->party_code) == 0)
            {
                party_list[i].is_locked = 1;
                found = 1;
                char msg[BUFFER_SIZE];
                snprintf(msg, sizeof(msg), "[SERVER] Party '%s' is now locked.\n", cli->party_code);
                send_to_client(cli, msg);
                break;
            }
        }

        if (!found)
        {
            send_to_client(cli, "[SERVER] Failed to find your party.\n");
        }

        return 1;
    }

    if (strcmp(command, "unlockparty") == 0 && cli->is_admin)
    {
        if (strlen(cli->party_code) == 0)
        {
            send_to_client(cli, "[SERVER] You are not in a party.\n");
            return 1;
        }

        int found = 0;
        for (int i = 0; i < party_count; ++i)
        {
            if (strcmp(party_list[i].code, cli->party_code) == 0)
            {
                party_list[i].is_locked = 0;
                found = 1;
                send_to_client(cli, "[SERVER] Party '%s' is now unlocked.\n", cli->party_code);
                break;
            }
        }

        if (!found)
        {
            send_to_client(cli, "[SERVER] Failed to find your party.\n");
        }

        return 1;
    }

    if (strcmp(command, "leaveparty") == 0)
    {
        if (strlen(cli->party_code) == 0)
        {
            send_to_client(cli, "[SERVER] You are not in a party.\n");
            return 1;
        }

        char left_party[PARTY_CODE_LEN];
        strncpy(left_party, cli->party_code, PARTY_CODE_LEN);
        left_party[PARTY_CODE_LEN - 1] = '\0';

        cli->party_code[0] = '\0';
        cli->invited_party[0] = '\0';

        send_to_client(cli, "[SERVER] You have left party '%s'.\n", left_party);

        int still_has_members = 0;
        for (int i = 0; i < MAX_CLIENTS; ++i)
        {
            if (clients[i] && strcmp(clients[i]->party_code, left_party) == 0)
            {
                still_has_members = 1;
                break;
            }
        }

        if (!still_has_members)
        {
            for (int i = 0; i < party_count; ++i)
            {
                if (strcmp(party_list[i].code, left_party) == 0)
                {

                    for (int j = i; j < party_count - 1; ++j)
                    {
                        party_list[j] = party_list[j + 1];
                    }
                    party_count--;
                    break;
                }
            }
        }

        return 1;
    }

    // === Admin Commands ===
    if (cli->is_admin)
    {
        if (strcmp(command, "kick") == 0)
            return kick_user(arg1, cli);
        if (strcmp(command, "pin") == 0)
        {
            char fullmsg[BUFFER_SIZE];
            snprintf(fullmsg, sizeof(fullmsg), "%s %s", arg1, arg2);

            if (strlen(fullmsg) == 0)
            {
                send_to_client(cli, "[SERVER] Usage: /pin <message>\n");
                return 1;
            }

            strncpy(pinned_message, fullmsg, BUFFER_SIZE);
            broadcast_system("A new message has been pinned.");
            return 1;
        }

        if (strcmp(command, "ban") == 0)
            return ban_user(arg1, cli);
        if (strcmp(command, "unban") == 0)
            return unban_user(arg1, cli);
        if (strcmp(command, "banlist") == 0)
            return list_banned(cli), 1;
        if (strcmp(command, "parties") == 0)
        {
            list_active_parties(cli);
            return 1;
        }

        if (strcmp(command, "mute") == 0)
            return mute_user(arg1, cli);
        if (strcmp(command, "unmute") == 0)
            return unmute_user(arg1, cli);
        if (strcmp(command, "mutelist") == 0)
            return list_muted(cli), 1;
        if (strcmp(command, "broadcast") == 0)
        {
            char fullmsg[BUFFER_SIZE];
            snprintf(fullmsg, sizeof(fullmsg), "%s %s", arg1, arg2);
            broadcast_system(arg1[0] ? fullmsg : "(empty)");
            return 1;
        }
        if (strcmp(command, "log") == 0)
            return send_log_to_client(cli), 1;
        if (strcmp(command, "shutdown") == 0)
        {
            broadcast_system("Server is shutting down.");
            exit(0);
        }
    }

    send_to_client(cli, "[SERVER] Unknown command: %s\n", command);
    return 1;
}
