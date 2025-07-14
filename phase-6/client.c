// client.c
// have included with emoji support inbuild input bar> and more features!
// too more to be added
//@mistabazz is going to add security in admin login
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
#include <time.h>
#include "pwdgen.h"
#include "emoji.h"

#define INPUT_BUFFER 1024
char input[INPUT_BUFFER];
int input_len = 0;
struct termios orig_termios;

#define LENGTH 2048

volatile sig_atomic_t flag = 0;
int sockfd = 0;
char username[32];
#define MAX_LOCAL_VANISH 100

typedef struct
{
    int id;
    int duration;
    char content[1024];
} vanish_display_t;

vanish_display_t vanish_messages[100];
int vanish_index = 0;

void str_trim_lf(char *arr, int length)
{
    for (int i = 0; i < length; i++)
    {
        if (arr[i] == '\n')
        {
            arr[i] = '\0';
            break;
        }
    }
}

void catch_ctrl_c_and_exit(int sig)
{
    (void)sig;
    flag = 1;
    write(STDOUT_FILENO, "\n\033[1;33m[CLIENT] Ctrl+C pressed, exiting...\033[0m\n", 47);
}

void disable_raw_mode()
{
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode()
{
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disable_raw_mode);

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ICANON | ECHO); // Disable canonical mode and echo
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void *recv_msg_handler(void *arg)
{
    (void)arg;
    char message[LENGTH] = {};

    while (1)
    {
        fd_set read_fds;
        struct timeval timeout;

        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int activity = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);

        if (activity > 0 && FD_ISSET(sockfd, &read_fds))
        {
            int receive = recv(sockfd, message, LENGTH, 0);
            if (receive > 0)
            {
                printf("\r\033[K"); // Clear current input line

                if (strncmp(message, "__PRIVATE__:", 12) == 0)
                {
                    char *from = strtok(message + 12, ":");
                    char *msg = strtok(NULL, "");
                    if (from && msg)
                        printf("\033[1;35m[PM from %s]: %s\033[0m\n", from, msg);
                    else
                        printf("\033[1;31m[ERROR parsing private message]\033[0m\n");
                }
                else if (strncmp(message, "__VANISH__:", 11) == 0)
                {
                    int id, duration;
                    char sender[32], msg[1024];

                    if (sscanf(message, "__VANISH__:%d:%d:%31[^:]: %[^\n]", &id, &duration, sender, msg) == 4)
                    {
                        char short_msg[512];
                        strncpy(short_msg, msg, sizeof(short_msg) - 1);
                        short_msg[sizeof(short_msg) - 1] = '\0';

                        printf("\033[1;33m[VANISH] %s: %s\033[0m (expires in %d sec)\n", sender, short_msg, duration);

                        if (vanish_index < 100)
                        {
                            vanish_messages[vanish_index].id = id;
                            vanish_messages[vanish_index].duration = duration;
                            snprintf(vanish_messages[vanish_index].content, sizeof(vanish_messages[vanish_index].content),
                                     "\033[1;33m[VANISH] %s: %s\033[0m (expires in %d sec)\n", sender, short_msg, duration);
                            vanish_index++;
                        }
                    }

                    memset(message, 0, sizeof(message));
                    continue;
                }
                else if (strncmp(message, "__EDIT__:", 9) == 0)
                {
                    int id;
                    char newmsg[BUFFER_SIZE];

                    if (sscanf(message, "__EDIT__:%d:%[^\n]", &id, newmsg) == 2)
                    {
                        for (int i = 0; i < vanish_index; ++i)
                        {
                            if (vanish_messages[i].id == id)
                            {
                                char shortmsg[1001];
                                strncpy(shortmsg, newmsg, 1000);
                                shortmsg[1000] = '\0';

                                snprintf(vanish_messages[i].content, sizeof(vanish_messages[i].content),
                                         "\033[1;34m[EDITED] %s\033[0m\n", shortmsg);

                                printf("\033[F\033[2K\r%s", vanish_messages[i].content);
                                fflush(stdout);
                                break;
                            }
                        }
                    }

                    memset(message, 0, sizeof(message));
                    continue;
                }
                else if (strncmp(message, "__DELETE__:", 11) == 0)
                {
                    int id;
                    if (sscanf(message, "__DELETE__:%d", &id) == 1)
                    {
                        for (int i = 0; i < vanish_index; ++i)
                        {
                            if (vanish_messages[i].id == id)
                            {
                                printf("\033[F\033[2K\r\033[1;31m[DELETED] Message ID %d has vanished.\033[0m\n", id);
                                fflush(stdout);
                                vanish_messages[i].content[0] = '\0'; // Invalidate
                                break;
                            }
                        }
                    }

                    memset(message, 0, sizeof(message));
                    continue;
                }
                else
                {
                    printf("%s\n", message);
                }

                // Redraw input prompt
                printf("> %s", input);
                fflush(stdout);
            }
            else if (receive == 0)
            {
                break; // server closed
            }
        }

        memset(message, 0, sizeof(message));
    }

    return NULL;
}

void chat_loop()
{
    fd_set readfds;
    unsigned char ch;

    while (1)
    {
        if (flag)
            break;

        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        FD_SET(sockfd, &readfds);
        int maxfd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;

        select(maxfd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            if (read(STDIN_FILENO, &ch, 1) <= 0)
                continue;

            if (ch == 127 || ch == 8)
            { // Backspace
                if (input_len > 0)
                {
                    input[--input_len] = '\0';
                    printf("\r\033[K> %s", input);
                    fflush(stdout);
                }
            }
            else if (ch == '\n')
            {
                input[input_len] = '\0';

                if (strcmp(input, "/exit") == 0)
                {
                    flag = 1;
                    break;
                }
                if (strncmp(input, "/emoji", 6) == 0)
                {
                    int num;
                    if (sscanf(input, "/emoji %d", &num) == 1)
                    {
                        const char *emoji = get_emoji_by_index(num);
                        if (emoji)
                        {
                            snprintf(input, INPUT_BUFFER, "%s", emoji); // overwrite input with emoji
                            send(sockfd, input, strlen(input), 0);      // send immediately
                        }
                        else
                        {
                            printf("Invalid emoji index.\n");
                        }
                    }
                    else
                    {
                        display_emoji_tab_with_index(); // show panel
                    }
                    input_len = 0;
                    input[0] = '\0';
                    printf("\r\033[K> ");
                    fflush(stdout);
                    continue;
                }

                else if (strcmp(input, "/clear") == 0)
                {
                    printf("\033[2J\033[H"); // Clear screen + move cursor to top-left
                    fflush(stdout);
                    input_len = 0;
                    input[0] = '\0';
                    printf("\r\033[K> ");
                    fflush(stdout);
                    continue; // skip sending to server
                }

                if (input_len > 0)
                {
                    send(sockfd, input, strlen(input), 0);
                }
                input_len = 0;
                input[0] = '\0';
                printf("\r\033[K> ");
                fflush(stdout);
            }

            else if (input_len < INPUT_BUFFER - 4)
            {
                input[input_len++] = ch;
                input[input_len] = '\0';
                printf("\r\033[K> %s", input);
                fflush(stdout);
            }
        }
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: %s <server_ip>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *ip = argv[1];
    int port = 9001;

    signal(SIGINT, catch_ctrl_c_and_exit);

    printf("Enter your username: ");
    fgets(username, 32, stdin);
    str_trim_lf(username, 32);

    if (strlen(username) < 2 || strlen(username) >= 32)
    {
        printf("Username must be between 2 and 31 characters.\n");
        return EXIT_FAILURE;
    }

    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        perror("Socket error");
        return EXIT_FAILURE;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connect error");
        return EXIT_FAILURE;
    }

    send(sockfd, username, 32, 0);

    if (strcmp(username, "admin") == 0)
    {
        for (int i = 0; i < MAX_TRIES; i++)
        {
            char res[200]; // for server response
            int rlen = recv(sockfd, res, 200, 0);
            if (rlen == 0)
            {
                printf("cannot receive!!\n");
                close(sockfd);
                return EXIT_FAILURE;
            }
            else if (strcmp(res, "ask") == 0)
            {
                char pwd[40];
                printf("Enter Admin Password: ");
                fflush(stdout);
                fgets(pwd, 39, stdin);
                str_trim_lf(pwd, 40);
                send(sockfd, pwd, 40, 0);
                char result[200];
                int res_len = recv(sockfd, result, 200, 0);
                if (res_len == 0)
                {
                    printf("Cannot receive!!\n");
                    close(sockfd);
                    return EXIT_FAILURE;
                }
                else if (strcmp(result, "true") == 0)
                {
                    printf("Enter Joining Alias: ");
                    fflush(stdout);
                    fgets(username, 32, stdin);
                    str_trim_lf(username, 32);
                    send(sockfd, username, sizeof(username), 0);
                    break;
                }
                else if (strcmp(result, "try") == 0)
                {
                    printf("\n %d tries left!!\n", MAX_TRIES - i - 1);
                    fflush(stdout);
                    // sleep(3); // wait for 'ask'
                }
                else
                {
                    printf("\n%s\nexiting..\n", result);
                    // sleep(5);
                    close(sockfd);
                    return EXIT_FAILURE;
                }
            }
            else
            {
                printf("\n%s\nexiting..\n", res);
                // sleep(5);
                close(sockfd);
                return EXIT_FAILURE;
            }
        }
        sleep(1); // for welcome msg
    }

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
