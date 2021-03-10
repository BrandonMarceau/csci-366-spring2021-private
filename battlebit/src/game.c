//
// Created by carson on 5/20/20.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "game.h"
#include "char_buff.h"

// STEP 9 - Synchronization: the GAME structure will be accessed by both players interacting
// asynchronously with the server.  Therefore the data must be protected to avoid race conditions.
// Add the appropriate synchronization needed to ensure a clean battle.

static game * GAME = NULL;

void game_init() {
    if (GAME) {
        free(GAME);
    }
    GAME = malloc(sizeof(game));
    GAME->status = CREATED;
    game_init_player_info(&GAME->players[0]);
    game_init_player_info(&GAME->players[1]);
}

void game_init_player_info(player_info *player_info) {
    player_info->ships = 0;
    player_info->hits = 0;
    player_info->shots = 0;
}

int game_fire(game *game, int player, int x, int y) {
    // Step 5 - This is the crux of the game.  You are going to take a shot from the given player and
    // update all the bit values that store our game state.
    //
    //  - You will need up update the players 'shots' value
    //  - you You will need to see if the shot hits a ship in the opponents ships value.  If so, record a hit in the
    //    current players hits field
    //  - If the shot was a hit, you need to flip the ships value to 0 at that position for the opponents ships field
    //
    //  If the opponents ships value is 0, they have no remaining ships, and you should set the game state to
    //  PLAYER_1_WINS or PLAYER_2_WINS depending on who won.

}

unsigned long long int xy_to_bitval(int x, int y) {
    // Step 1 - implement this function.  We are taking an x, y position
    // and using bitwise operators, converting that to an unsigned long long
    // with a 1 in the position corresponding to that x, y
    //
    // x:0, y:0 == 0b00000...0001 (the one is in the first position)
    // x:1, y: 0 == 0b00000...10 (the one is in the second position)
    // ....
    // x:0, y: 1 == 0b100000000 (the one is in the eighth position)
    //
    // you will need to use bitwise operators and some math to produce the right
    // value.
    if (y < 0 || y > 7) {
        return 0;
    }
    if (x < 0 || x > 7) {
        return 0;
    }
    else {
        return 1ull << (x + (8 * y));
    }
}

struct game * game_get_current() {
    return GAME;
}

int game_load_board(struct game *game, int player, char * spec) {
    // Step 2 - implement this function.  Here you are taking a C
    // string that represents a layout of ships, then testing
    // to see if it is a valid layout (no off-the-board positions
    // and no overlapping ships)
    //

    // if it is valid, you should write the corresponding unsigned
    // long long value into the Game->players[player].ships data
    // slot and return 1
    //
    // if it is invalid, you should return -1


    // Check to make sure string of ships is not empty and is appropriate size to include all ships

    player_info *current_player = &GAME->players[player];

    if (spec == NULL) {
        return -1;
    }
    if (strlen(spec) != 15) {
        return -1;
    }

    bool carrier = false;
    bool battleship = false;
    bool destroyer = false;
    bool submarine = false;
    bool patrol = false;
    for (int i = 0; i < 15; i += 3) {
        if (spec[i] == 67 || spec[i] == 99) { carrier = true; }
        if (spec[i] == 66 || spec[i] == 98) { battleship = true; }
        if (spec[i] == 68 || spec[i] == 100) { destroyer = true; }
        if (spec[i] == 83 || spec[i] == 115) { submarine = true; }
        if (spec[i] == 80 || spec[i] == 112) { patrol = true; }
    }
    if (carrier == false || battleship == false || destroyer == false || submarine == false || patrol == false) {
        return -1;
    }

    for (int i = 0; i < 15; i += 3) {
        if (spec[i] > 64 && spec[i] < 91) {
            if (add_ship_horizontal(current_player, spec[i + 1] - '0', spec[i + 2] - '0', get_ship_length(spec[i])) == -1) {
                return -1;
            }
            else {
                add_ship_horizontal(current_player, spec[i + 1] - '0', spec[i + 2] - '0', get_ship_length(spec[i]));
            }
        }
        if (spec[i] > 96 && spec[i] < 123) {
            if (add_ship_vertical(current_player, spec[i + 1] - '0', spec[i + 2] - '0', get_ship_length(spec[i])) == -1) {
                return -1;
                }
            else {
                add_ship_vertical(current_player, spec[i + 1] - '0', spec[i + 2] - '0', get_ship_length(spec[i]));
                }
            }
        }


        if (!game->players[player].ships || !game->players[(player + 1) % 2].ships) {
            game->status = CREATED;
        } else if (player == 0) {
            game->status = PLAYER_1_TURN;
        } else {
            game->status = PLAYER_0_TURN;
        }
        return 1;
    }





    int add_ship_horizontal(player_info *player, int x, int y, int length) {
        // implement this as part of Step 2
        // returns 1 if the ship can be added, -1 if not
        // hint: this can be defined recursively
        if (length == 0) {
            return 1;
        }
        if (x < 0 || x > 7 || y < 0 || y > 7) {
            return -1;
        }
        else {
            unsigned long long mask = xy_to_bitval(x, y);
            if (player->ships & mask) {
                return -1;
            }
            player->ships = mask | player->ships;
            return add_ship_horizontal(player, ++x, y, --length);
        }
    }

        int add_ship_vertical(player_info *player, int x, int y, int length) {
            // implement this as part of Step 2
            // returns 1 if the ship can be added, -1 if not
            // hint: this can be defined recursively
            if (length == 0) {
                return 1;
            }
            if (x < 0 || x > 7 || y < 0 || y > 7) {
                return -1;
            }
            else {
                unsigned long long mask = xy_to_bitval(x, y);
                if (player->ships & mask) {
                        return -1;
                    }
                player->ships = mask | player->ships;
                return add_ship_vertical(player, x, ++y, --length);
                }
            }

            int get_ship_length(type) {
                switch (type) {
                    case 'C':
                    case 'c':
                        return 5;
                    case 'B':
                    case 'b':
                        return 4;
                    case 'D':
                    case 'd':
                    case 'S':
                    case 's':
                        return 3;
                    case 'P':
                    case 'p':
                        return 2;
                }
}
