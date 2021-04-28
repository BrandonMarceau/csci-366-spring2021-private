//
// Created by carson on 5/20/20.
//

#include "stdio.h"
#include "stdlib.h"
#include "server.h"
#include "char_buff.h"
#include "game.h"
#include "repl.h"
#include "pthread.h"
#include<string.h>    //strlen
#include<sys/socket.h>
#include<arpa/inet.h>    //inet_addr
#include<unistd.h>    //write

static game_server *SERVER;
static pthread_mutex_t lock;

void init_server() {
    if (SERVER == NULL) {
        SERVER = calloc(1, sizeof(struct game_server));

    } else {
        printf("Server already started");
    }
}

int handle_client_connect(int player) {
    // STEP 8 - This is the big one: you will need to re-implement the REPL code from
    // the repl.c file, but with a twist: you need to make sure that a player only
    // fires when the game is initialized and it is there turn.  They can broadcast
    // a message whenever, but they can't just shoot unless it is their turn.
    //
    // The commands will obviously not take a player argument, as with the system level
    // REPL, but they will be similar: load, fire, etc.
    //
    // You must broadcast informative messages after each shot (HIT! or MISS!) and let
    // the player print out their current board state at any time.
    //
    // This function will end up looking a lot like repl_execute_command, except you will
    // be working against network sockets rather than standard out, and you will need
    // to coordinate turns via the game::status field.

    // Initialize Mutex Lock
    pthread_mutex_init(&lock, NULL);

    int client_socket_fd = SERVER->player_sockets[player];
    int read_size;
    char raw_buffer[2000];
    char_buff *input_buffer = cb_create(2000);
    char_buff *output_buffer = cb_create(2000);


    if (player == 0) {
        cb_append(output_buffer, "Welcome to the battleBit server Player 0");
    } else {
        cb_append(output_buffer, "Welcome to the battleBit server Player 1");
    }
    cb_append(output_buffer, "\nbattleBit (? for help) > ");
    cb_write(client_socket_fd, output_buffer);


    while ((read_size = recv(client_socket_fd, raw_buffer, 2000, 0)) > 0) {
        // Clear the input and output buffers
        cb_reset(output_buffer);
        cb_reset(input_buffer);

        pthread_mutex_lock(&lock);

        if (read_size > 0) {
            raw_buffer[read_size] = '\0';   // null terminate read

            // Append to the input buffer
            cb_append(input_buffer, raw_buffer);

            // Tokenize the input find out what needs to be done
            char *command = cb_tokenize(input_buffer, " \r\n");
            char *arg1 = cb_next_token(input_buffer);
            char *arg2 = cb_next_token(input_buffer);
            if (strcmp(command, "?") == 0) {
                // Create Output Menu
                cb_append(output_buffer, "? - show help\n");
                cb_append(output_buffer, "load <string> - load a ship layout file for the given player\n");
                cb_append(output_buffer, "show - shows the board for the given player\n");
                cb_append(output_buffer, "fire [0-7] [0-7] - fire at the given position\n");
                cb_append(output_buffer, "say <string> - Send the string to all players as part of a chat\n");
                cb_append(output_buffer, "exit - quit the server\n");

                // Print Menu
                cb_write(client_socket_fd, output_buffer);

            } else if (strcmp(command, "load") == 0) {
                // Load player's ships onto game board
                game_load_board(game_get_current(), player, arg1);

                // Check if both players have loaded their boards
                if (game_get_current()->players[0].ships != 0 && game_get_current()->players[1].ships != 0) {
                    game_get_current()->status = PLAYER_0_TURN;
                    cb_append(output_buffer, "\nAll Player Boards Loaded");
                    cb_append(output_buffer, "\nPlayer 0 Turn");
                    server_broadcast(output_buffer, player);
                } else if (game_get_current()->players[0].ships != 0 && game_get_current()->players[1].ships == 0) {
                    cb_append(output_buffer, "Waiting on Player 1");
                    cb_write(client_socket_fd, output_buffer);
                } else if (game_get_current()->players[1].ships != 0 && game_get_current()->players[0].ships == 0) {
                    cb_append(output_buffer, "\nWaiting on Player 0");
                    cb_write(client_socket_fd, output_buffer);
                }


            } else if (strcmp(command, "show") == 0) {
                // Show the player's board
                repl_print_board(game_get_current(), player, output_buffer);
                cb_write(client_socket_fd, output_buffer);

            } else if (strcmp(command, "fire") == 0) {
                // Check that the game has begun
                if (game_get_current()->status == CREATED) {
                    cb_append(output_buffer, "Game Has Not Begun!");
                    server_broadcast(output_buffer, player);

                    // Check if player's turn
                } else if (player == 0 && game_get_current()->status == PLAYER_1_TURN) {
                    cb_append(output_buffer, "Player 1 Turn");
                    cb_write(client_socket_fd, output_buffer);

                } else if (player == 1 && game_get_current()->status == PLAYER_0_TURN) {
                    cb_append(output_buffer, "Player 0 Turn");
                    cb_write(client_socket_fd, output_buffer);

                } else {
                    // set new game status
                    if (game_get_current()->status == PLAYER_0_TURN) {
                        game_get_current()->status = PLAYER_1_TURN;
                    } else {
                        game_get_current()->status = PLAYER_0_TURN;
                    }

                    // Take a shot at the opposing player's board
                    int outmsg = game_fire(game_get_current(), player, atoi(arg1), atoi(arg2));
                    char *msg = malloc(sizeof(char) * 500);

                    // print out the results
                    if (outmsg == 0) {
                        sprintf(msg, "\nPlayer %d fires at %s %s - MISS", player, arg1, arg2);

                    } else if (outmsg == 1) {
                        sprintf(msg, "\nPlayer %d fires at %s %s - HIT ", player, arg1, arg2);
                        // If the shot ends the game, print message
                        if (game_get_current()->status == PLAYER_0_WINS ||
                            game_get_current()->status == PLAYER_1_WINS) {
                            sprintf(msg + strlen(msg), "- PLAYER %d WINS", player);
                        }
                    }
                    cb_append(output_buffer, msg);
                    server_broadcast(output_buffer, player);
                    free(msg);
                }

            } else if (strcmp(command, "say") == 0) {
                // Send message to all players
                cb_append(output_buffer, "\nPlayer ");
                cb_append_int(output_buffer, player);
                cb_append(output_buffer, " says: ");
                cb_append(output_buffer, arg1);
                cb_append(output_buffer, " ");
                cb_append(output_buffer, arg2);
                server_broadcast(output_buffer, player);

            } else if (strcmp(command, "shortcut") == 0) {
                // place a "ship" of size one in (0, 0)
                game_load_board(game_get_current(), 1, "Z00");

            } else if (strcmp(command, "exit") == 0) {
                close(client_socket_fd);

            } else if (command != NULL) {
                // Create output
                cb_append(output_buffer, "Invalid Command : ");
                cb_append(output_buffer, command);
                // output it
                cb_write(client_socket_fd, output_buffer);
            }
            cb_reset(output_buffer);
            cb_append(output_buffer, "\nbattleBit (? for help) > ");
            cb_write(client_socket_fd, output_buffer);
        }

        pthread_mutex_unlock(&lock);

        // Check to see if game is over, and close the socket if either player has won
        if (game_get_current()->status == PLAYER_0_WINS || game_get_current()->status == PLAYER_1_WINS) {
            close(client_socket_fd);
        }
    }
}

void server_broadcast(char_buff *msg, int player) {
    // send message to all players

    // This helps formatting issues in terminal
    char_buff *prompt = cb_create(100);
    cb_append(prompt, "\nbattleBit (? for help) > ");
    for (int i = 0; i < 2; i++) {
        cb_write(SERVER->player_sockets[i], msg);
        if (i == ((player + 1) % 2)) {
            cb_write(SERVER->player_sockets[i], prompt);
        }
    }
    cb_print(msg);
}


int run_server() {
    // STEP 7 - implement the server code to put this on the network.
    // Here you will need to initalize a server socket and wait for incoming connections.
    //
    // When a connection occurs, store the corresponding new client socket in the SERVER.player_sockets array
    // as the corresponding player position.
    //
    // You will then create a thread running handle_client_connect, passing the player number out
    // so they can interact with the server asynchronously

    int server_socket_fd = socket(AF_INET,
                                  SOCK_STREAM,
                                  IPPROTO_TCP);
    if (server_socket_fd == -1) {
        printf("Could not create socket\n");
    }

    int yes = 1;
    setsockopt(server_socket_fd,
               SOL_SOCKET,
               SO_REUSEADDR, &yes, sizeof(yes));

    struct sockaddr_in server;

    // fill out the socket information
    server.sin_family = AF_INET;
    // bind the socket on all available interfaces
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(9876);

    int request = 0;
    if (bind(server_socket_fd,
            // Again with the cast
             (struct sockaddr *) &server,
             sizeof(server)) < 0) {
        puts("Bind failed");
    } else {
        puts("Bind worked!");
        listen(server_socket_fd, 3);

        //Accept an incoming connection
        puts("Waiting for incoming connections...");


        struct sockaddr_in client;
        socklen_t size_from_connect;
        int client_socket_fd;
        int request_count = 0;
        int player = 0;

        while ((client_socket_fd = accept(server_socket_fd, (struct sockaddr *) &client, &size_from_connect)) > 0) {
            SERVER->player_sockets[player] = client_socket_fd;
            // create pthread to handle player interaction
            pthread_create(&SERVER->player_threads[player], NULL, (void *)handle_client_connect, player);
            player++;
            if (player > 1) {
              break;
            }
        }
    }
}

int server_start() {
    // STEP 7 - using a pthread, run the run_server() function asynchronously, so you can still
    // interact with the game via the command line REPL

    init_server();
    pthread_create(&SERVER->server_thread, NULL, (void *)run_server, NULL);

}

