#include "game.h"
#include "utils.h"
#include "client.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include "common.h"

extern client_t *clients[MAX_CLIENTS];
guess_game_t guess_game = {0};

void start_guess_game(client_t *cli) {
    if (guess_game.active) {
        send_to_client(cli, "[GAME] A game is already in progress!\n");
        return;
    }

    srand(time(NULL));
    guess_game.active = 1;
    guess_game.number_to_guess = (rand() % 100) + 1;
    guess_game.start_time = time(NULL);
    strncpy(guess_game.started_by, cli->username, USERNAME_LEN);
    strncpy(guess_game.current_turn, cli->username, USERNAME_LEN);

    broadcast_message("[GAME]Number Guessing Game started! Guess a number between 1 and 100 using /guess <num>", NULL);

    char msg[128];
    snprintf(msg, sizeof(msg), "[GAME] %s will guess first. Use /guess <number>", cli->username);
    broadcast_message(msg, NULL);
}

void handle_guess_command(client_t *cli, const char *arg) {
    if (!guess_game.active) {
        send_to_client(cli, "[GAME] No active game. Start one with /guessgame start\n");
        return;
    }

    if (strcmp(cli->username, guess_game.current_turn) != 0) {
        send_to_client(cli, "[GAME] It's not your turn. Wait for your turn.\n");
        return;
    }

    int guess = atoi(arg);
    if (guess <= 0 || guess > 100) {
        send_to_client(cli, "[GAME] Invalid number. Guess between 1 and 100.\n");
        return;
    }

    char result_msg[256];

    if (guess == guess_game.number_to_guess) {
        snprintf(result_msg, sizeof(result_msg), "[GAME] %s guessed the correct number: %d!", cli->username, guess);
        broadcast_message(result_msg, NULL);
        guess_game.active = 0;
        guess_game.current_turn[0] = '\0';
        return;
    } else if (guess < guess_game.number_to_guess) {
        snprintf(result_msg, sizeof(result_msg), "[GAME] %s guessed too low!", cli->username);
    } else {
        snprintf(result_msg, sizeof(result_msg), "[GAME] %s guessed too high!", cli->username);
    }

    broadcast_message(result_msg, NULL);

    // Switch turn
    int found_next = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i] &&
            clients[i]->active &&
            strlen(clients[i]->username) > 0 &&
            strcmp(clients[i]->username, cli->username) != 0) {

            strncpy(guess_game.current_turn, clients[i]->username, USERNAME_LEN);
            found_next = 1;
            break;
        }
    }

    if (found_next) {
        char turn_msg[128];
        snprintf(turn_msg, sizeof(turn_msg), "[GAME] %s will guess next. Use /guess <number>", guess_game.current_turn);
        broadcast_message(turn_msg, NULL);
    } else {
        // Fallback to current player
        strncpy(guess_game.current_turn, cli->username, USERNAME_LEN);
        send_to_client(cli, "[GAME] No other players found. You can guess again.\n");
    }
}
