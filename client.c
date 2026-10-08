// client.c
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
#include <time.h>
#include "common.h"
#include "emoji.h"
#include "e2ee.h"
#include "file_transfer.h"
#include "crypto_utils.h"

#define INPUT_BUFFER 1024
#define LENGTH 4096
#define MAX_LOCAL_VANISH 100
#define HISTORY_MAX 50
#define MAX_TRIES 5

char input[INPUT_BUFFER];
int input_len = 0;
struct termios orig_termios;

volatile sig_atomic_t flag = 0;
int sockfd = 0;
char username[32];

typedef struct
{
    int id;
    int duration;
    char content[1024];
} vanish_display_t;

vanish_display_t vanish_messages[MAX_LOCAL_VANISH];
int vanish_index = 0;

static char history[HISTORY_MAX][INPUT_BUFFER];
static int history_count = 0;
static int history_pos = 0;

static const char *tab_commands[] = {
    "/help", "/msg", "/e2ee", "/sendfile", "/fileaccept", "/filedecline",
    "/whoami", "/users", "/ping", "/time", "/uptime", "/motd", "/clear",
    "/lastseen", "/setcolor", "/colorlist", "/createparty", "/joinparty",
    "/party", "/leaveparty", "/guessgame", "/guess", "/vanish", "/emoji",
    "/exit", "/kick", "/ban", "/unban", "/banlist", "/mute", "/unmute",
    "/mutelist", "/setadminpwd", "/broadcast", "/pin", "/setmotd", "/log",
    "/shutdown", "/parties", "/lockparty", "/invite", "/partyinfo", "/whois",
    NULL
};

static void history_add(const char *cmd)
{
    if (!cmd || strlen(cmd) == 0) return;
    if (history_count > 0 && strcmp(history[(history_count - 1) % HISTORY_MAX], cmd) == 0)
    {
        history_pos = history_count;
        return;
    }
    snprintf(history[history_count % HISTORY_MAX], INPUT_BUFFER, "%s", cmd);
    history_count++;
    history_pos = history_count;
}

void str_trim_lf(char *arr, int length)
{
    for (int i = 0; i < length; i++)
    {
        if (arr[i] == '\n' || arr[i] == '\r')
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
    ssize_t w = write(STDOUT_FILENO, "\n\033[1;33m[CLIENT] Ctrl+C pressed, exiting...\033[0m\n", 47);
    (void)w;
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
    raw.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void handle_received_line(char *line)
{
    str_trim_lf(line, strlen(line));
    if (strlen(line) == 0) return;

    if (strncmp(line, "__KEYRESP__:", 12) == 0)
    {
        char target[32] = {0};
        char pubhex[128] = {0};
        if (sscanf(line, "__KEYRESP__:%31[^:]:%127s", target, pubhex) == 2)
        {
            if (strcmp(pubhex, "NOT_FOUND") != 0)
            {
                unsigned char raw_pub[X25519_KEY_LEN];
                if (hex_decode(pubhex, raw_pub, sizeof(raw_pub)) == X25519_KEY_LEN)
                {
                    e2ee_get_or_create_session(target, raw_pub);
                }
            }
        }
        return;
    }

    if (strncmp(line, "__E2EE__:", 9) == 0)
    {
        char from[USERNAME_LEN] = {0};
        char plaintext[BUFFER_SIZE] = {0};
        int seq = 0;
        if (e2ee_decrypt_packet(line, from, plaintext, sizeof(plaintext), &seq) == 0)
        {
            printf("\033[1;32m🔒 [E2EE from %s (ratchet #%d)]: %s\033[0m\n", from, seq, plaintext);
        }
        else
        {
            printf("\033[1;31m[E2EE ERROR] Failed to decrypt Double Ratchet message!\033[0m\n");
        }
        return;
    }

    if (strncmp(line, "__FILEREQ__:", 12) == 0)
    {
        int fid = 0;
        char from[32] = {0}, filename[256] = {0}, sha[SHA256_HEX_LEN] = {0};
        size_t fsize = 0;
        if (sscanf(line, "__FILEREQ__:%d:%31[^:]:%255[^:]:%zu:%64s",
                   &fid, from, filename, &fsize, sha) == 5)
        {
            file_transfer_handle_request(fid, from, filename, fsize, sha);
        }
        return;
    }

    if (strncmp(line, "__FILEACCEPT__:", 15) == 0)
    {
        int fid = 0;
        char from[32] = {0};
        if (sscanf(line, "__FILEACCEPT__:%d:%31s", &fid, from) == 2)
        {
            file_transfer_handle_accept(fid, sockfd);
        }
        return;
    }

    if (strncmp(line, "__FILEDECLINE__:", 16) == 0)
    {
        int fid = 0;
        char from[32] = {0};
        if (sscanf(line, "__FILEDECLINE__:%d:%31s", &fid, from) == 2)
        {
            printf("\033[1;33m[FILE] %s declined transfer #%d.\033[0m\n", from, fid);
        }
        return;
    }

    if (strncmp(line, "__FILECHUNK__:", 14) == 0)
    {
        int fid = 0, cseq = 0, ctot = 0;
        char b64[FILE_CHUNK_SIZE * 2 + 128] = {0};
        if (sscanf(line, "__FILECHUNK__:%d:%d:%d:%4095s", &fid, &cseq, &ctot, b64) == 4)
        {
            file_transfer_receive_chunk(fid, cseq, ctot, b64);
        }
        return;
    }

    if (strncmp(line, "__FILEDONE__:", 13) == 0)
    {
        int fid = 0;
        if (sscanf(line, "__FILEDONE__:%d", &fid) == 1)
        {
            file_transfer_complete(fid);
        }
        return;
    }

    if (strncmp(line, "__TYPING__:", 11) == 0)
    {
        char typing_user[32] = {0};
        if (sscanf(line, "__TYPING__:%31s", typing_user) == 1)
        {
            printf("\033[2m[%s is typing...]\033[0m\n", typing_user);
        }
        return;
    }

    if (strncmp(line, "__PRIVATE__:", 12) == 0)
    {
        char *from = strtok(line + 12, ":");
        char *msg = strtok(NULL, "");
        if (from && msg)
            printf("\033[1;35m[PM from %s]: %s\033[0m\n", from, msg);
        else
            printf("\033[1;31m[ERROR parsing private message]\033[0m\n");
        return;
    }

    if (strncmp(line, "__VANISH__:", 11) == 0)
    {
        int id, duration;
        char sender[32], msg[1024];

        if (sscanf(line, "__VANISH__:%d:%d:%31[^:]: %1023[^\n]", &id, &duration, sender, msg) == 4)
        {
            char short_msg[512];
            strncpy(short_msg, msg, sizeof(short_msg) - 1);
            short_msg[sizeof(short_msg) - 1] = '\0';

            printf("\033[1;33m[VANISH] %s: %s\033[0m (expires in %d sec)\n", sender, short_msg, duration);

            if (vanish_index < MAX_LOCAL_VANISH)
            {
                vanish_messages[vanish_index].id = id;
                vanish_messages[vanish_index].duration = duration;
                snprintf(vanish_messages[vanish_index].content, sizeof(vanish_messages[vanish_index].content),
                         "\033[1;33m[VANISH] %s: %s\033[0m (expires in %d sec)\n", sender, short_msg, duration);
                vanish_index++;
            }
        }
        return;
    }

    if (strncmp(line, "__EDIT__:", 9) == 0)
    {
        int id;
        char newmsg[BUFFER_SIZE];

        if (sscanf(line, "__EDIT__:%d:%2047[^\n]", &id, newmsg) == 2)
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
        return;
    }

    if (strncmp(line, "__DELETE__:", 11) == 0)
    {
        int id;
        if (sscanf(line, "__DELETE__:%d", &id) == 1)
        {
            for (int i = 0; i < vanish_index; ++i)
            {
                if (vanish_messages[i].id == id)
                {
                    printf("\033[F\033[2K\r\033[1;31m[DELETED] Message ID %d has vanished.\033[0m\n", id);
                    fflush(stdout);
                    vanish_messages[i].content[0] = '\0';
                    break;
                }
            }
        }
        return;
    }

    // Standard message
    printf("%s\n", line);
}

void *recv_msg_handler(void *arg)
{
    (void)arg;
    char buffer[LENGTH] = {};

    while (1)
    {
        if (flag)
            break;

        fd_set read_fds;
        struct timeval timeout;

        FD_ZERO(&read_fds);
        FD_SET(sockfd, &read_fds);
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;

        int activity = select(sockfd + 1, &read_fds, NULL, NULL, &timeout);

        if (activity > 0 && FD_ISSET(sockfd, &read_fds))
        {
            int receive = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
            if (receive > 0)
            {
                buffer[receive] = '\0';
                printf("\r\033[K"); // Clear prompt line

                // Split by newline and handle each line
                char *saveptr = NULL;
                char *line = strtok_r(buffer, "\n", &saveptr);
                while (line != NULL)
                {
                    handle_received_line(line);
                    line = strtok_r(NULL, "\n", &saveptr);
                }

                // Redraw prompt
                printf("> %s", input);
                fflush(stdout);
            }
            else
            {
                flag = 1;
                printf("\r\033[K\033[1;31m[CLIENT] Server disconnected.\033[0m\n");
                break;
            }
        }

        memset(buffer, 0, sizeof(buffer));
    }

    return NULL;
}

void *file_ticker_thread(void *arg)
{
    (void)arg;
    while (!flag)
    {
        file_transfer_sender_tick(sockfd);
        usleep(15000); // 15ms pace
    }
    return NULL;
}

void chat_loop()
{
    fd_set readfds;
    unsigned char ch;
    time_t last_typing_sent = 0;

    while (1)
    {
        if (flag)
            break;

        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 50000;

        int sel = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &tv);
        if (sel <= 0)
            continue;

        if (FD_ISSET(STDIN_FILENO, &readfds))
        {
            if (read(STDIN_FILENO, &ch, 1) <= 0)
                continue;

            // Handle Escape sequences (Arrow keys)
            if (ch == 27)
            {
                unsigned char seq1 = 0, seq2 = 0;
                if (read(STDIN_FILENO, &seq1, 1) > 0 && read(STDIN_FILENO, &seq2, 1) > 0)
                {
                    if (seq1 == '[')
                    {
                        if (seq2 == 'A') // Up Arrow -> Previous history
                        {
                            if (history_pos > 0)
                            {
                                history_pos--;
                                strncpy(input, history[history_pos % HISTORY_MAX], sizeof(input) - 1);
                                input[sizeof(input) - 1] = '\0';
                                input_len = strlen(input);
                                printf("\r\033[K> %s", input);
                                fflush(stdout);
                            }
                            continue;
                        }
                        else if (seq2 == 'B') // Down Arrow -> Next history
                        {
                            if (history_pos < history_count)
                            {
                                history_pos++;
                                if (history_pos == history_count)
                                {
                                    input[0] = '\0';
                                    input_len = 0;
                                }
                                else
                                {
                                    strncpy(input, history[history_pos % HISTORY_MAX], sizeof(input) - 1);
                                    input[sizeof(input) - 1] = '\0';
                                    input_len = strlen(input);
                                }
                                printf("\r\033[K> %s", input);
                                fflush(stdout);
                            }
                            continue;
                        }
                    }
                }
                continue;
            }

            // Tab Completion
            if (ch == '\t')
            {
                if (input_len > 0 && input[0] == '/')
                {
                    const char *match = NULL;
                    int matches = 0;
                    for (int i = 0; tab_commands[i] != NULL; ++i)
                    {
                        if (strncmp(tab_commands[i], input, input_len) == 0)
                        {
                            match = tab_commands[i];
                            matches++;
                        }
                    }
                    if (matches == 1 && match)
                    {
                        snprintf(input, sizeof(input), "%s ", match);
                        input_len = strlen(input);
                        printf("\r\033[K> %s", input);
                        fflush(stdout);
                    }
                    else if (matches > 1)
                    {
                        printf("\n\033[2mSuggestions: ");
                        for (int i = 0; tab_commands[i] != NULL; ++i)
                        {
                            if (strncmp(tab_commands[i], input, input_len) == 0)
                            {
                                printf("%s ", tab_commands[i]);
                            }
                        }
                        printf("\033[0m\n> %s", input);
                        fflush(stdout);
                    }
                }
                continue;
            }

            // Backspace
            if (ch == 127 || ch == 8)
            {
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
                history_add(input);

                if (strcmp(input, "/exit") == 0)
                {
                    flag = 1;
                    break;
                }

                if (strncmp(input, "/e2ee ", 6) == 0)
                {
                    char target[32] = {0};
                    char plaintext[BUFFER_SIZE] = {0};
                    if (sscanf(input + 6, "%31s %2047[^\n]", target, plaintext) == 2)
                    {
                        ratchet_session_t *s = e2ee_find_session(target);
                        if (!s || !s->active)
                        {
                            char kreq[128];
                            snprintf(kreq, sizeof(kreq), "__KEYREQ__:%s\n", target);
                            send(sockfd, kreq, strlen(kreq), 0);
                            for (int w = 0; w < 20; w++)
                            {
                                usleep(50000);
                                s = e2ee_find_session(target);
                                if (s && s->active) break;
                            }
                        }

                        char e2ee_packet[4096];
                        if (e2ee_encrypt_message(target, plaintext, e2ee_packet, sizeof(e2ee_packet)) == 0)
                        {
                            char send_buf[sizeof(e2ee_packet) + 32];
                            snprintf(send_buf, sizeof(send_buf), "%s\n", e2ee_packet);
                            send(sockfd, send_buf, strlen(send_buf), 0);
                            printf("\033[1;32m🔒 [E2EE to %s]: %s\033[0m\n", target, plaintext);
                        }
                        else
                        {
                            printf("\033[1;31m[E2EE ERROR] Could not establish Double Ratchet session with '%s' (user not found or no public key).\033[0m\n", target);
                        }
                    }
                    else
                    {
                        printf("[E2EE] Usage: /e2ee <username> <message>\n");
                    }
                    input_len = 0;
                    input[0] = '\0';
                    printf("\r\033[K> ");
                    fflush(stdout);
                    continue;
                }

                if (strncmp(input, "/sendfile ", 10) == 0)
                {
                    char target[32] = {0};
                    char filepath[256] = {0};
                    if (sscanf(input + 10, "%31s %255s", target, filepath) == 2)
                    {
                        file_transfer_start_send(target, filepath, sockfd);
                    }
                    else
                    {
                        printf("[FILE] Usage: /sendfile <username> <filepath>\n");
                    }
                    input_len = 0;
                    input[0] = '\0';
                    printf("\r\033[K> ");
                    fflush(stdout);
                    continue;
                }

                if (strncmp(input, "/fileaccept ", 12) == 0)
                {
                    int fid = 0;
                    if (sscanf(input + 12, "%d", &fid) == 1)
                    {
                        file_transfer_accept(fid, sockfd);
                    }
                    else
                    {
                        printf("[FILE] Usage: /fileaccept <transfer_id>\n");
                    }
                    input_len = 0;
                    input[0] = '\0';
                    printf("\r\033[K> ");
                    fflush(stdout);
                    continue;
                }

                if (strncmp(input, "/filedecline ", 13) == 0)
                {
                    int fid = 0;
                    if (sscanf(input + 13, "%d", &fid) == 1)
                    {
                        file_transfer_decline(fid, sockfd);
                    }
                    else
                    {
                        printf("[FILE] Usage: /filedecline <transfer_id>\n");
                    }
                    input_len = 0;
                    input[0] = '\0';
                    printf("\r\033[K> ");
                    fflush(stdout);
                    continue;
                }

                if (strncmp(input, "/emoji", 6) == 0)
                {
                    int num;
                    if (sscanf(input, "/emoji %d", &num) == 1)
                    {
                        const char *emoji = get_emoji_by_index(num);
                        if (emoji)
                        {
                            snprintf(input, INPUT_BUFFER, "%s", emoji);
                            send(sockfd, input, strlen(input), 0);
                        }
                        else
                        {
                            printf("Invalid emoji index.\n");
                        }
                    }
                    else
                    {
                        display_emoji_tab_with_index();
                    }
                    input_len = 0;
                    input[0] = '\0';
                    printf("\r\033[K> ");
                    fflush(stdout);
                    continue;
                }

                if (strcmp(input, "/clear") == 0)
                {
                    printf("\033[2J\033[H");
                    fflush(stdout);
                    input_len = 0;
                    input[0] = '\0';
                    printf("\r\033[K> ");
                    fflush(stdout);
                    continue;
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

                // Send typing packet if user is typing text (not a command)
                if (input[0] != '/' && (time(NULL) - last_typing_sent >= 3))
                {
                    send(sockfd, "__TYPING__\n", 11, 0);
                    last_typing_sent = time(NULL);
                }
            }
        }
    }
}

static int client_recv_line(int s, char *buf, size_t max_len)
{
    size_t idx = 0;
    while (idx < max_len - 1)
    {
        char c;
        int r = recv(s, &c, 1, 0);
        if (r <= 0) break;
        buf[idx++] = c;
        if (c == '\n') break;
    }
    buf[idx] = '\0';
    return (int)idx;
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
    signal(SIGPIPE, SIG_IGN);

    printf("Enter your username: ");
    if (fgets(username, sizeof(username), stdin) == NULL)
    {
        return EXIT_FAILURE;
    }
    str_trim_lf(username, sizeof(username));

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

    send(sockfd, username, strlen(username), 0);

    if (strcmp(username, "admin") == 0)
    {
        for (int i = 0; i < MAX_TRIES; i++)
        {
            char res[200];
            int rlen = client_recv_line(sockfd, res, sizeof(res));
            if (rlen <= 0)
            {
                printf("Cannot receive from server!!\n");
                close(sockfd);
                return EXIT_FAILURE;
            }
            str_trim_lf(res, sizeof(res));

            if (strcmp(res, "ask") == 0)
            {
                char pwd[64];
                printf("Enter Admin Password: ");
                fflush(stdout);
                if (fgets(pwd, sizeof(pwd), stdin) == NULL)
                {
                    close(sockfd);
                    return EXIT_FAILURE;
                }
                str_trim_lf(pwd, sizeof(pwd));
                send(sockfd, pwd, strlen(pwd), 0);

                char result[200];
                int res_len = client_recv_line(sockfd, result, sizeof(result));
                if (res_len <= 0)
                {
                    printf("Cannot receive from server!!\n");
                    close(sockfd);
                    return EXIT_FAILURE;
                }
                str_trim_lf(result, sizeof(result));

                if (strcmp(result, "true") == 0)
                {
                    printf("Enter Joining Alias: ");
                    fflush(stdout);
                    if (fgets(username, sizeof(username), stdin) == NULL)
                    {
                        close(sockfd);
                        return EXIT_FAILURE;
                    }
                    str_trim_lf(username, sizeof(username));
                    send(sockfd, username, strlen(username), 0);
                    break;
                }
                else if (strcmp(result, "try") == 0)
                {
                    printf("\n %d tries left!!\n", MAX_TRIES - i - 1);
                    fflush(stdout);
                }
                else
                {
                    printf("\n%s\nexiting..\n", result);
                    close(sockfd);
                    return EXIT_FAILURE;
                }
            }
            else
            {
                printf("\n%s\nexiting..\n", res);
                close(sockfd);
                return EXIT_FAILURE;
            }
        }
        sleep(1);
    }

    // Initialize cryptographic Double Ratchet and file transfer engines
    e2ee_init();
    file_transfer_init();

    // Register our Double Ratchet identity public key with server
    char my_pub_hex[128];
    hex_encode(g_e2ee.identity_pub, X25519_KEY_LEN, my_pub_hex, sizeof(my_pub_hex));
    char key_pkt[256];
    snprintf(key_pkt, sizeof(key_pkt), "__IDKEY__:%s\n", my_pub_hex);
    send(sockfd, key_pkt, strlen(key_pkt), 0);

    printf("\033[1;32m[CLIENT] Connected. Type /help for commands, /exit to quit.\033[0m\n");
    printf("> ");
    fflush(stdout);

    enable_raw_mode();

    pthread_t recv_thread, file_thread;
    pthread_create(&recv_thread, NULL, recv_msg_handler, NULL);
    pthread_create(&file_thread, NULL, file_ticker_thread, NULL);

    chat_loop();

    disable_raw_mode();

    printf("\n\033[1;31m[CLIENT] Disconnected.\033[0m\n");
    file_transfer_cleanup();
    e2ee_cleanup();
    close(sockfd);
    return EXIT_SUCCESS;
}
