//
// Created by carson on 5/20/20.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "repl.h"
#include "server.h"
#include "char_buff.h"

struct char_buff * repl_read_command(char * prompt) {
    printf("%s", prompt);
    char *line = NULL;
    size_t buffer_size = 0; // let getline autosize it
    if (getline(&line, &buffer_size, stdin) == -1) {
        if (feof(stdin)) {
            exit(EXIT_SUCCESS);  // We received an EOF
        } else  {
            perror("readline");
            exit(EXIT_FAILURE);
        }
    }
    if (strcmp(line, "") == 0) {
        free(line);
        return NULL;
    } else {
        struct char_buff *buffer = cb_create(2000);
        cb_append(buffer, line);
        free(line);
        return buffer;
    }
}

void repl_execute_command(struct char_buff * buffer) {
    char* command = cb_tokenize(buffer, " \n");
    if (command) {
        char* arg1 = cb_next_token(buffer);
        char* arg2 = cb_next_token(buffer);
        char* arg3 = cb_next_token(buffer);
        if (strcmp(command, "exit") == 0) {
            printf("goodbye!");
            exit(EXIT_SUCCESS);
        } else if(strcmp(command, "?") == 0) {
            printf("? - show help\n");
            printf("load [0-1] <string> - load a ship layout file for the given player\n");
            printf("show [0-1] - shows the board for the given player\n");
            printf("fire [0-1] [0-7] [0-7] - fire at the given position\n");
            printf("say <string> - Send the string to all players as part of a chat\n");
            printf("reset - reset the game\n");
            printf("server - start the server\n");
            printf("exit - quit the server\n");
        } else if(strcmp(command, "server") == 0) {
            server_start();
        } else if(strcmp(command, "show") == 0) {

            struct char_buff *boardBuffer = cb_create(2000);
            repl_print_board(game_get_current(), atoi(arg1), boardBuffer);
            printf("%s", boardBuffer->buffer);
            cb_free(boardBuffer);

        } else if(strcmp(command, "reset") == 0) {

            game_init();

        } else if (strcmp(command, "load") == 0) {

            int player = atoi(arg1);
            game_load_board(game_get_current(), player, arg2);

        } else if (strcmp(command, "fire") == 0) {
            int player = atoi(arg1);
            int x = atoi(arg2);
            int y = atoi(arg3);
            if (x < 0 || x >= BOARD_DIMENSION || y < 0 || y >= BOARD_DIMENSION) {
                printf("Invalid coordinate: %i %i\n", x, y);
            } else {
                printf("Player %i fired at %i %i\n", player + 1, x, y);
                int result = game_fire(game_get_current(), player, x, y);
                if (result) {
                    printf("  HIT!!!");
                } else {
                    printf("  Miss");
                }
            }

        } else if (strcmp(command, "shortcut") == 0) {
            game_load_board(game_get_current(), 1, "Z00");

        } else {
            printf("Unknown Command: %s\n", command);
        }
    }
}

void repl_print_board(game *game, int player, char_buff * buffer) {
    player_info player_info = game->players[player];
    cb_append(buffer, "battleBit.........\n");
    cb_append(buffer, "-----[ ENEMY ]----\n");
    repl_print_hits(&player_info, buffer);
    cb_append(buffer, "==================\n");
    cb_append(buffer, "-----[ SHIPS ]----\n");
    repl_print_ships(&player_info, buffer);
    cb_append(buffer, ".........battleBit\n\n");
}

void repl_print_ships(player_info *player_info, char_buff *buffer) {
    // Step 3 - Implement this to print out the visual ships representation
    //  for the console.  You will need to use bit masking for each position
    //  to determine if a ship is at the position or not.  If it is present
    //  you need to print an X.  If not, you need to print a space character ' '

    //char_buff *ships_buffer = cb_create(2000);
    cb_append(buffer, "  0 1 2 3 4 5 6 7 \n");
    for (int y = 0; y < 8; ++y) {
        cb_append_int(buffer, y);
        for (int x = 0; x < 8; ++x) {
            unsigned long long int mask = xy_to_bitval(x, y);
            // Append a * character on the buffer if the spot the mask is checking has a player ship (player ships and mask = 1)
            if (player_info->ships & mask) {
                cb_append(buffer, " *");
            }
                // Append a space on the buffer if the spot the mask is checking has no ship there (player ships = 0)
            else {
                cb_append(buffer, "  ");
            }
        }
        cb_append(buffer, " \n");
    }


}

void repl_print_hits(struct player_info *player_info, struct char_buff *buffer) {
    // Step 4 - Implement this to print out a visual representation of the shots
    // that the player has taken and if they are a hit or not.  You will again need
    // to use bit-masking, but this time you will need to consult two values: both
    // hits and shots values in the players game struct.  If a shot was fired at
    // a given spot and it was a hit, print 'H', if it was a miss, print 'M'.  If
    // no shot was taken at a position, print a space character ' '

    //char_buff *hits_buffer = cb_create(2000);
    cb_append(buffer, "  0 1 2 3 4 5 6 7 \n");
    for (int y = 0; y < 8; ++y) {
        cb_append_int(buffer, y);
        for (int x = 0; x < 8; ++x) {
            unsigned long long int mask = xy_to_bitval(x, y);
            // Append an H onto buffer if the player has taken a shot at a space and hit a ship
            if (player_info->hits & mask) {
                cb_append(buffer, " H");
            }
                // Append an M onto the buffer if the player has taken a shot at that space and NOT hit a ship
            else if (player_info->shots & mask) {
                cb_append(buffer, " M");
            }
                // Blank space if the player has not taken a shot at that space
            else {
                cb_append(buffer, "  ");
            }
        }
        cb_append(buffer, " \n");
    }
}