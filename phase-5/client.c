// client.c 
//commented some important point for you guyzzzzzzzzzzzz...!
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <signal.h>
#include <termios.h>
#include <sys/select.h>
#include "common.h"

#define INPUT_BUFFER 1024
char input[INPUT_BUFFER];
int input_len = 0;
struct termios orig_termios;

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

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void *recv_msg_handler(void *arg) {
    char message[LENGTH] = {};
    while (1) {
        int receive = recv(sockfd, message, LENGTH, 0);
        if (receive > 0) {
            // Clear input line
            printf("\r\033[K%s\n", message);
            // Redraw current input line
            printf("> %s", input);
            fflush(stdout);
        } else if (receive == 0) {
            break;
        }
        memset(message, 0, sizeof(message));
    }
    return NULL;
}

void chat_loop() {
    fd_set readfds;
    char ch;

    while (1) {
        if (flag) break;

        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sockfd, &readfds);
        int maxfd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;

        select(maxfd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (read(STDIN_FILENO, &ch, 1) <= 0) continue;

            if (ch == 127 || ch == 8) {  // Backspace
                if (input_len > 0) {
                    input[--input_len] = '\0';
                    printf("\r\033[K> %s", input);
                    fflush(stdout);
                }
            } else if (ch == '\n') {
                input[input_len] = '\0';
                if (strcmp(input, "/exit") == 0) {
                    flag = 1;
                    break;
                }
                if (input_len > 0) {
                    send(sockfd, input, input_len, 0);
                }
                input_len = 0;
                input[0] = '\0';
                printf("\r\033[K> ");
                fflush(stdout);
            } else if (ch >= 32 && ch <= 126 && input_len < INPUT_BUFFER - 1) {
                input[input_len++] = ch;
                input[input_len] = '\0';
                printf("\r\033[K> %s", input);
                fflush(stdout);
            }
        }
    }
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

    // Send username
    send(sockfd, username, 32, 0);

    printf("\033[1;32m[CLIENT] Connected. Type /exit to quit.\033[0m\n");
    printf("> ");
    fflush(stdout);

    enable_raw_mode();

    pthread_t recv_thread;
    pthread_create(&recv_thread, NULL, recv_msg_handler, NULL);

    chat_loop();

    disable_raw_mode();

    printf("\n\033[1;31m[CLIENT] Disconnected.\033[0m\n");

    close(sockfd);
    return EXIT_SUCCESS;
}
