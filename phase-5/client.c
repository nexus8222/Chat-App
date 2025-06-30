// client.c (Final Phase 5 version)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include "common.h"
#define LENGTH 2048

volatile sig_atomic_t flag = 0;
int sockfd = 0;
char username[32];

void str_trim_lf(char *arr, int length) {
    for (int i = 0; i < length; i++) {
        if (arr[i] == '\n') {
            arr[i] = '\0';
            break;
        }
    }
}

void catch_ctrl_c_and_exit(int sig) {
    flag = 1;
}

void *send_msg_handler(void *arg) {
    char message[LENGTH] = {};
    while (1) {
        fgets(message, LENGTH, stdin);
        str_trim_lf(message, LENGTH);

        if (strcmp(message, "/exit") == 0) {
            break;
        } else {
            send(sockfd, message, strlen(message), 0);
        }
        bzero(message, LENGTH);
    }
    flag = 1;
    return NULL;
}

void *recv_msg_handler(void *arg) {
    char message[LENGTH] = {};
    while (1) {
        int receive = recv(sockfd, message, LENGTH, 0);
        if (receive > 0) {
            printf("%s", message);
            fflush(stdout);
        } else if (receive == 0) {
            break;
        }
        memset(message, 0, sizeof(message));
    }
    return NULL;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <server_ip>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *ip = argv[1];
    int port = 9001;

    signal(SIGINT, catch_ctrl_c_and_exit);

    printf("Enter your username: ");
    fgets(username, 32, stdin);
    str_trim_lf(username, 32);

    if (strlen(username) < 2 || strlen(username) >= 32) {
        printf("Username must be between 2 and 31 characters.\n");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Socket error");
        return EXIT_FAILURE;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connect error");
        return EXIT_FAILURE;
    }

    send(sockfd, username, 32, 0);

    printf("\033[1;32m[CLIENT] Connected. Type /exit to quit.\033[0m\n");

    pthread_t send_thread, recv_thread;
    pthread_create(&send_thread, NULL, send_msg_handler, NULL);
    pthread_create(&recv_thread, NULL, recv_msg_handler, NULL);

    while (1) {
        if (flag) {
            printf("\n\033[1;31m[CLIENT] Disconnected.\033[0m\n");
            break;
        }
    }

    close(sockfd);
    return EXIT_SUCCESS;
}
