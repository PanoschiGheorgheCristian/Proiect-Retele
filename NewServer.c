#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

#define MAX_CLIENTS 2
#define BOARD_SIZE 8
#define BLANK_SPACE ' '
#define PIECE_1 'X'
#define PIECE_2 'O'

struct leaderboard_entry {
    char name[256];
    int wins;
};

void error(const char *msg) {
    perror(msg);
    exit(1);
}

void init_board(char board[BOARD_SIZE][BOARD_SIZE]) {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = BLANK_SPACE;
        }
    }

    board[BOARD_SIZE/2 - 1][BOARD_SIZE/2 - 1] = PIECE_2;
    board[BOARD_SIZE/2][BOARD_SIZE/2] = PIECE_2;
    board[BOARD_SIZE/2 - 1][BOARD_SIZE/2] = PIECE_1;
    board[BOARD_SIZE/2][BOARD_SIZE/2 - 1] = PIECE_1;
}

int check_other_pieces(char board[BOARD_SIZE][BOARD_SIZE], int row, int col, char piece) {
    for(int direction = 0; direction <= 7; direction++) {
        int found_same_piece = 0;
        int found_different_piece = 0;
        int move_x;
        int move_y;
        int updated_x = row;
        int updated_y = col;

        if(direction % 4 == 0)
            move_x = 0;
        else if(direction < 4)
            move_x = -1;
        else
            move_x = 1;

        if(direction % 4 == 2)
            move_y = 0;
        else if(direction > 2 && direction < 6)
            move_y = -1;
        else
            move_y = 1;
        
        updated_x = updated_x + move_x;
        updated_y = updated_y + move_y;

        while(updated_x < BOARD_SIZE && updated_x >= 0 &&
        updated_y < BOARD_SIZE && updated_y >= 0 ) {
            if(board[updated_x][updated_y] == BLANK_SPACE)
                break;
            if(board[updated_x][updated_y] != piece)
                found_different_piece = 1;
            else {
                found_same_piece = 1;
                break;
            }

            updated_x = updated_x + move_x;
            updated_y = updated_y + move_y;
        }

        if(found_different_piece == 1 && found_same_piece == 1)
            return 1;
    }
    return 0;
}

int check_valid_move(char board[BOARD_SIZE][BOARD_SIZE], int row, int col, char piece) {
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        return 0;
    }

    if (board[row][col] != BLANK_SPACE) {
        return 0;
    }

    if(check_other_pieces(board, row, col, piece) == 1)
        return 1;
    else    
        return 0;

}

void place_piece(char board[BOARD_SIZE][BOARD_SIZE], int row, int col, char piece) {
    board[row][col] = piece;
    for(int direction = 0; direction <= 7; direction++) {
        int found_same_piece = 0;
        int move_x;
        int move_y;
        int updated_x = row;
        int updated_y = col;

        if(direction % 4 == 0)
            move_x = 0;
        else if(direction < 4)
            move_x = -1;
        else
            move_x = 1;

        if(direction % 4 == 2)
            move_y = 0;
        else if(direction > 2 && direction < 6)
            move_y = -1;
        else
            move_y = 1;
        
        updated_x = updated_x + move_x;
        updated_y = updated_y + move_y;

        while(updated_x < BOARD_SIZE && updated_x >= 0 &&
        updated_y < BOARD_SIZE && updated_y >= 0 ) {
            if(board[updated_x][updated_y] == piece) {
                found_same_piece = 1;
                break;
            }
            if(board[updated_x][updated_y] == BLANK_SPACE)
                break;
            
            updated_x = updated_x + move_x;
            updated_y = updated_y + move_y;
        }

        if(found_same_piece == 1) {
            updated_x = row + move_x;
            updated_y = col + move_y;

            while (board[updated_x][updated_y] != piece)
            {
                board[updated_x][updated_y] = piece;
                updated_x = updated_x + move_x;
                updated_y = updated_y + move_y;
            }
            
        }
    }
}

void print_leaderboard(struct leaderboard_entry *leaderboard, int entries) {
    for(int i=0; i < entries; i++) {
        printf("\n %s", leaderboard[i].name);
        printf("%d \n", leaderboard[i].wins);
    }
}

int possible_moves(char board[BOARD_SIZE][BOARD_SIZE], char piece) {
    for(int i = 0; i <= BOARD_SIZE; i++)
        for(int j = 0; j <= BOARD_SIZE; j++)
            if(check_valid_move(board, i, j, piece))
                return 1;
    return 0;
}

int count_pieces(char board[BOARD_SIZE][BOARD_SIZE], char piece) {
    int count = 0;
    for(int i = 0; i <= BOARD_SIZE; i++)
        for(int j = 0; j <= BOARD_SIZE; j++)
            if(board[i][j] == piece)
               count++;
    return count;
}

int end_game(char board[BOARD_SIZE][BOARD_SIZE], char piece) {
    if(possible_moves(board, piece) == 0)
        return 1;
    return 0;
}

void board_to_string(char board[BOARD_SIZE][BOARD_SIZE], char* elements) {
    char board_string[65];
    for(int i = 0; i <= BOARD_SIZE; i++)
        for(int j = 0; j <= BOARD_SIZE; j++)
            board_string[8 * i + j] = board[i][j];
    strcpy(elements, board_string);
} 

int main(int argc, char *argv[]) {
    int listen_socket, client_socket;
    socklen_t client_len;
    struct sockaddr_in server_address, client_address;
    pid_t pid;
    int n;
    char buffer[256];
    int clients[MAX_CLIENTS];
    int clients_count = 0;
    int i;
    int turn = 1;
    int game_end = 0;
    char board[BOARD_SIZE][BOARD_SIZE];
    char board_elements[65];
    struct leaderboard_entry leaderboard[100];
    int leaderboard_players = 0;
    int player_1 = -1;
    int player_2 = -1;
    char name_1[100];
    char name_2[100];

    listen_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket < 0)
        error("ERROR opening socket");
    
    memset((char *) &server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl (INADDR_ANY);
    server_address.sin_port = htons(2024);


    if (bind(listen_socket, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
        error("ERROR on binding");
    
    listen(listen_socket,5);
    client_len = sizeof(client_address);


    while (1) {

        client_socket = accept(listen_socket, (struct sockaddr *) &client_address, &client_len);
        if (client_socket < 0)
            error("ERROR on accept");

        if (clients_count >= MAX_CLIENTS) {
            close(client_socket);
            continue;
        }

        clients[clients_count] = client_socket;
        clients_count++;


        if (clients_count == MAX_CLIENTS) {
            pid = fork();

            if (pid < 0) {
                error("ERROR on fork");
            }

            if (pid == 0) {

                close(listen_socket);
                memset(buffer, 0, sizeof(buffer));

                int score1 = 0;
                int score2 = 0;
                player_1 = -1;
                player_2 = -1;
                int start_game_1 = 1;
                int start_game_2 = 1;

                write(clients[0], "1", 2);
                write(clients[1], "2", 2);


                bzero(buffer,256);

                init_board(board);

                while(game_end == 0) {

                    if(turn == 1) {
                        if(start_game_1 == 1) {
                        read(clients[0], buffer, 255);

                        for(int i=0; i < leaderboard_players; i++)
                            if(strcmp(leaderboard[i].name, buffer) == 0)
                                player_1 = i;
                        if(player_1 == -1) {
                            leaderboard_players++;
                            strcpy(leaderboard[leaderboard_players - 1].name, buffer);
                            leaderboard[leaderboard_players - 1].wins = 0;
                            player_1 = leaderboard_players - 1;
                        }

                        start_game_1 = 0;
                        bzero(buffer,256);
                        }

                        if(end_game(board, PIECE_1)) {
                            game_end = 1;
                            score1 = count_pieces(board, PIECE_1);
                            score2 = count_pieces(board, PIECE_2);
                            if(score1 > score2)
                                leaderboard[player_1].wins++;
                            else
                                leaderboard[player_2].wins++;
                            print_leaderboard(leaderboard, leaderboard_players);
                        }
                        else {
                            int end_turn = 0;
                            while(end_turn == 0) {
                                board_to_string(board, board_elements);
                                n = write(clients[0], board_elements, 65);
                                bzero(board_elements,65);
                                if (n < 0) error("ERROR reading from socket");

                                n = read(clients[0], buffer, 255);
                                if (n < 0) error("ERROR reading from socket");
                                int number_read = atoi(buffer);
                                if(number_read == 0) printf("Client didn't write a valid move");

                                int row = (number_read - 1) / BOARD_SIZE;
                                int col = (number_read - 1) % BOARD_SIZE;

                                if(check_valid_move(board, row, col, PIECE_1)) {
                                    place_piece(board, row, col, PIECE_1);
                                    end_turn = 1;
                                }

                                turn = 2;
                                bzero(buffer,256);
                            }
                        }
                    }
                    else {
                        if(start_game_2 == 1) {
                        read(clients[1], buffer, 255);
                        for(int i=0; i < leaderboard_players; i++)
                            if(strcmp(leaderboard[i].name, buffer) == 0)
                                player_2 = i;
                        if(player_2 == -1) {
                            leaderboard_players++;
                            strcpy(leaderboard[leaderboard_players - 1].name, buffer);
                            leaderboard[leaderboard_players - 1].wins = 0;
                            player_2 = leaderboard_players - 1;
                        }

                        start_game_2 = 0;
                        bzero(buffer,256);
                        }

                        if(end_game(board, PIECE_1)) {
                            game_end = 1;
                            score1 = count_pieces(board, PIECE_1);
                            score2 = count_pieces(board, PIECE_2);
                            if(score1 > score2)
                                leaderboard[player_1].wins++;
                            else
                                leaderboard[player_2].wins++;
                            print_leaderboard(leaderboard, leaderboard_players);
                        }
                        else {
                            int end_turn = 0;
                            while(end_turn == 0) {
                                board_to_string(board, board_elements);
                                n = write(clients[1], board_elements, 65);
                                bzero(board_elements,65);
                                if (n < 0) error("ERROR reading from socket");

                                n = read(clients[1], buffer, 255);
                                if (n < 0) error("ERROR reading from socket");
                                int number_read = atoi(buffer);
                                if(number_read == 0) printf("Client didn't write a valid move");

                                int row = (number_read - 1) / BOARD_SIZE;
                                int col = (number_read - 1) % BOARD_SIZE;
                                if(check_valid_move(board, row, col, PIECE_2)) {
                                    place_piece(board, row, col, PIECE_2);
                                    end_turn = 1;
                                }

                                turn = 1;
                                bzero(buffer,256);
                            }
                        }
                    }
                }

                close(clients[0]);
                close(clients[1]);
                exit(0);
            }

            for (i = 0; i < MAX_CLIENTS; i++) {
                close(clients[i]);
            }
            clients_count = 0;
        }
    }
}
