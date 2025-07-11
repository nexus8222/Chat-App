// server.c (Echo-to-sender fixed) ----> we're not getting our msges back so i modified the server!
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <time.h>
#include "common.h"
#include "lastseen.h"
#include "vanish.h"
#include "pwdgen.h"
#include "client.h"
#include "commands.h"
#include "admin.h"
#include "log.h"
#include "banlist.h"
#include "utils.h"
#include "party.h"

#define PORT 9001
char pinned_message[BUFFER_SIZE] = "";

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
time_t server_start_time;
void *vanish_cleaner_thread(void *arg)
{
    (void)arg;
    while (1)
    {
        check_and_expire_vanish_messages();
        sleep(1);
    }
    return NULL;
}


void party_broadcast_system(const char *msg, int except_sock)
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i] && clients[i]->sockfd != except_sock)
        {
            send(clients[i]->sockfd, msg, strlen(msg), 0);
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void add_client(client_t *cl)
{
    pthread_mutex_lock(&clients_mutex);
    strcpy(cl->color, "\033[0;37m");

    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (!clients[i])
        {
            clients[i] = cl;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}
client_t *get_client_by_name(const char *name)
{
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i] && strcmp(clients[i]->username, name) == 0)
        {
            return clients[i];
        }
    }
    return NULL;
}

void remove_client(int sockfd)
{
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i] && clients[i]->sockfd == sockfd)
        {
            clients[i] = NULL;
            break;
        }
    }
    pthread_mutex_unlock(&clients_mutex);
}

void *handle_client(void *arg)
{
    char buffer[2048];
    char name[32];
    client_t *cli = (client_t *)arg;

    int rlen = recv(cli->sockfd, name, sizeof(name), 0);
    if (rlen <= 0)
    {
        close(cli->sockfd);
        free(cli);
        pthread_detach(pthread_self());
        return NULL;
    }

    trim_newline(name);
    strncpy(cli->username, name, sizeof(cli->username));
    pthread_mutex_lock(&clients_mutex);
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (clients[i] && strcmp(clients[i]->username, name) == 0)
        {
            send_to_client(cli, "\033[1;31m[SERVER] Username already taken.\033[0m\n");
            close(cli->sockfd);
            free(cli);
            pthread_mutex_unlock(&clients_mutex);
            pthread_exit(NULL);
        }
    }
    if (strncmp(buffer, "__PRIVATE__:", 12) == 0)
    {
        char *target = strtok(buffer + 12, ":");
        char *msg = strtok(NULL, "");

        if (!target || !msg)
            return NULL;

        client_t *receiver = get_client_by_name(target);
        if (receiver)
        {
            char outbuf[BUFFER_SIZE];

            snprintf(outbuf, sizeof(outbuf), "[PM from %s]: %s", cli->username, msg);
            send(receiver->sockfd, outbuf, strlen(outbuf), 0);

            // Optional: echo back to sender for confirmation
            snprintf(outbuf, sizeof(outbuf), "[PM to %s]: %s", target, msg);
            send(cli->sockfd, outbuf, strlen(outbuf), 0);
        }
        else
        {
            char failmsg[128];
            snprintf(failmsg, sizeof(failmsg), "[SYSTEM] User '%s' not found.", target);
            send(cli->sockfd, failmsg, strlen(failmsg), 0);
        }
        return NULL;
    }

    if (strcmp(cli->username, "admin") == 0)
    {
        cli->is_admin = 1;
        // check for password file
        char pwd[40];
        char dpwd[40];
        char enc_pwd[40];

        // if any error in getting password 1 will close connection
        int close_flag = 0;
        FILE *pfile = fopen("pwd.env", "r");
        if (pfile == NULL)
        {
            close_flag = 1;
            printf("Server dosen't have password!!\n");
            send_to_client(cli, "Server dosen't have password!!\n");
        }
        int res = fscanf(pfile, "%s", enc_pwd);
        if (res == 0)
        {
            close_flag = 1;
            printf("server can't get password from file\n");
            send_to_client(cli, "contact server!!\n");
        }
        else if (pwddecrypt(enc_pwd, dpwd) != 1)
        {
            close_flag = 1;
            printf("server can't decrypt password!!\n");
            send_to_client(cli, "contact server!!\n");
        }

        if (close_flag == 1)
        {
            fclose(pfile);
            close(cli->sockfd);
            free(cli);
            pthread_detach(pthread_self());
            pthread_mutex_unlock(&clients_mutex);
            return NULL;
        }

        for (int i = 0; i < MAX_TRIES; i++)
        {
            sleep(1);
            send(cli->sockfd, "ask", 4, 0);
            // sleep(5);  // before receiving password
            int plen = recv(cli->sockfd, pwd, sizeof(pwd), 0);
            if (plen <= 0)
            {
                // no response from client
                close_flag = 1;
                break;
            }
            else if (strcmp(dpwd, pwd) != 0)
            {
                if (i == MAX_TRIES - 1)
                {
                    close_flag = 1;
                    send_to_client(cli, "password wrong disconnecting...");
                    break;
                }
                else
                {
                    sleep(1);
                    send(cli->sockfd, "try", 4, 0);
                    // after 'try' client.c file wait for 'ask'
                    // to get pasaword again
                }
            }
            else
            {
                cli->is_admin = 1;
                sleep(1);
                send(cli->sockfd, "true", 5, 0);
                // after 'true' client.c file gets original username
                char uname[40];
                sleep(1); // sleep before recieving
                int ulen = recv(cli->sockfd, uname, sizeof(uname), 0);
                if (ulen <= 0)
                {
                    close_flag = 1;
                }
                strcpy(cli->username, uname);
                break;
            }
        }
        fclose(pfile);

        if (close_flag == 1)
        {
            close(cli->sockfd);
            free(cli);
            pthread_detach(pthread_self());
            pthread_mutex_unlock(&clients_mutex);
            return NULL;
        }
    }
    pthread_mutex_unlock(&clients_mutex);

    cli->join_time = time(NULL);
    inet_ntop(AF_INET, &cli->address.sin_addr, cli->ip, INET_ADDRSTRLEN);
    log_connection(cli->username, cli->ip);
    if (cli->is_admin)
        printf("%s joined as admin\n", cli->username);
    else
        printf("%s joined as user\n", cli->username);

    fflush(stdout);
    strncpy(cli->party_code, "public", PARTY_CODE_LEN);

    add_client(cli);

    char join_msg[128];
    snprintf(join_msg, sizeof(join_msg), "\033[1;32m[JOIN] %s has entered the chat.\033[0m\n", cli->username);
    party_broadcast_system(join_msg, cli->sockfd);

    send_to_client(cli, "\033[1;36mWelcome! Type /help for commands.\033[0m\n");
    send_to_client(cli, "\033[1;34m--- Message of the Day ---\033[0m\n");
    handle_command("/motd", cli);
    if (strlen(pinned_message) > 0)
    {
        send_to_client(cli,
                       "\n\033[1;33m=== PINNED MESSAGE ===\033[0m\n"
                       "%s\n"
                       "\033[1;33m======================\033[0m\n",
                       pinned_message);
    }

    while (1)
    {
        int rlen = recv(cli->sockfd, buffer, sizeof(buffer), 0);
        if (rlen <= 0)
            break;

        buffer[rlen] = '\0';
        trim_newline(buffer);
        if (strlen(buffer) == 0)
            continue;

        if (cli->is_muted)
        {
            send_to_client(cli, "\033[1;31m[SERVER] You are muted.\033[0m\n");
            continue;
        }

        if (buffer[0] == '/')
        {
            if (handle_command(buffer, cli))
                continue; // command handled
        }
        else
        {
            char msg[2048];
            snprintf(msg, sizeof(msg), "%s[%s]:%s %s\n",
                     cli->color,
                     cli->username,
                     COLOR_RESET,
                     buffer);
            broadcast_message(msg, cli);
            strncpy(cli->last_msg, buffer, BUFFER_SIZE);
            cli->last_msg_time = time(NULL);
            ;
        }
    }

    char leave_msg[128];
    snprintf(leave_msg, sizeof(leave_msg), "\033[1;31m[LEAVE] %s has left the chat.\033[0m\n", cli->username);
    party_broadcast_system(leave_msg, cli->sockfd);
    log_disconnection(cli->username, cli->ip);
    update_lastseen(cli->username);
    save_lastseen_to_file();

    cli->active = 0;
    close(cli->sockfd);
    remove_client(cli->sockfd);

    free(cli);
    pthread_detach(pthread_self());
    return NULL;
}

int main()
{
    FILE *pfile = fopen("pwd.env", "r");
    if (pfile == NULL)
    {
        printf("creating admin password for the first time!!\n");
        char pwd[40];
        char dpwd[40];
        char enc_pwd[40];
        printf("enter password: ");
        fgets(pwd, 39, stdin);
        trim_newline(pwd);
        int dflag = pwdencrypt(pwd, enc_pwd);
        int eflag = pwddecrypt(enc_pwd, dpwd);
        FILE *pfile2 = fopen("pwd.env", "w");
        if (pfile2 == NULL)
        {
            printf("file can't be created!\n");
            return EXIT_FAILURE;
        }
        printf("\n eflag : encrypter func value if 1 then ok\n");
        printf("dflag = %d, eflag = %d\n", dflag, eflag);
        printf("\n enc pass: %s", enc_pwd);
        printf("\n dec pass: %s\n", dpwd);
        int r = fprintf(pfile2, "%s", enc_pwd);
        printf("n = %d\n", r);
        if (r == 0)
        {
            printf("password can't be written in file\n");
            return EXIT_FAILURE;
        }
        printf("pwd file successfully generated\n");
        fclose(pfile2);
    }
    else
    {
        fclose(pfile);
    }
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0)
    {
        perror("Socket failed");
        exit(1);
    }

    struct sockaddr_in server_addr, cli_addr;
    socklen_t cli_len = sizeof(cli_addr);

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    signal(SIGPIPE, SIG_IGN);

    if (bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        return 1;
    }

    if (listen(listener, 10) < 0)
    {
        perror("Listen failed");
        return 1;
    }

    printf("Server started on port %d\n", PORT);
    log_server_start(PORT);
    load_banlist();
    server_start_time = time(NULL);
    init_vanish_module();
    pthread_t vanish_thread;
    pthread_create(&vanish_thread, NULL, vanish_cleaner_thread, NULL);

    while (1)
    {
        int new_sock = accept(listener, (struct sockaddr *)&cli_addr, &cli_len);
        if (new_sock < 0)
        {
            perror("Accept failed");
            continue;
        }

        char cli_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &cli_addr.sin_addr, cli_ip, INET_ADDRSTRLEN);

        if (is_ip_banned(cli_ip))
        {
            printf("Rejected banned IP: %s\n", cli_ip);
            close(new_sock);
            continue;
        }

        client_t *cli = (client_t *)malloc(sizeof(client_t));
        memset(cli, 0, sizeof(client_t));
        cli->active = 1;
        cli->sockfd = new_sock;
        cli->address = cli_addr;
        cli->is_admin = 0;
        cli->is_muted = 0;
        cli->last_seen = time(NULL);
        strncpy(cli->party_code, "public", PARTY_CODE_LEN);

        pthread_t tid;
        pthread_create(&tid, NULL, handle_client, (void *)cli);
    }

    return 0;
}
