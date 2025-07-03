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

#include "client.h"
#include "commands.h"
#include "admin.h"
#include "log.h"
#include "banlist.h"
#include "utils.h"
#include "party.h"

#define PORT 9001

client_t *clients[MAX_CLIENTS];
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;
time_t server_start_time;

void trim_newline(char *s)
{
    int len = strlen(s);
    if (len > 0 && s[len - 1] == '\n')
        s[len - 1] = '\0';
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
    if (strcmp(cli->username, "admin") == 0)
    {
        cli->is_admin = 1;
    }
    pthread_mutex_unlock(&clients_mutex);

    cli->join_time = time(NULL);
    inet_ntop(AF_INET, &cli->address.sin_addr, cli->ip, INET_ADDRSTRLEN);
    log_connection(cli->username, cli->ip);

    strncpy(cli->party_code, "public", PARTY_CODE_LEN);

    add_client(cli);

    char join_msg[128];
    snprintf(join_msg, sizeof(join_msg), "\033[1;32m[JOIN] %s has entered the chat.\033[0m\n", cli->username);
    party_broadcast_system(join_msg, cli->sockfd);

    send_to_client(cli, "\033[1;36mWelcome! Type /help for commands.\033[0m\n");
    send_to_client(cli, "\033[1;34m--- Message of the Day ---\033[0m\n");
    handle_command("/motd", cli);

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
            snprintf(msg, sizeof(msg), "\033[1m[%s]:\033[0m %s\n", cli->username, buffer);
            broadcast_message(msg, cli);
        }
    }

    char leave_msg[128];
    snprintf(leave_msg, sizeof(leave_msg), "\033[1;31m[LEAVE] %s has left the chat.\033[0m\n", cli->username);
    party_broadcast_system(leave_msg, cli->sockfd);
    log_disconnection(cli->username, cli->ip);
    update_lastseen(cli->username);
    save_lastseen_to_file();

    close(cli->sockfd);
    remove_client(cli->sockfd);
    free(cli);
    pthread_detach(pthread_self());
    return NULL;
}

int main()
{
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
