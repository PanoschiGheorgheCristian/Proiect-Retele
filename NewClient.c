#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <arpa/inet.h>

void error(const char *msg) {
    perror(msg);
    exit(0);
}

void show_board(char board_string[65]) {
    for(int i = 0; i < 8; i++) {
        for(int j=0; j < 8; j++)
            printf(" %c", board_string[i * 8 + j]);
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    int sockfd, n;
    struct sockaddr_in serv_addr;
    char buffer[256];
    int player;
    char pieces;

    if (argc < 3) {
        fprintf(stderr,"usage %s hostname port\n", argv[0]);
        exit(0);
    }
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));
    
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) 
        error("ERROR connecting");

    read(sockfd,buffer,255);
    if(strstr(buffer, "1")) {
        player = 1;
        pieces = 'X';
    }
    else if(strstr(buffer, "2")) {
        player = 2;
        pieces = 'O';
    }

    printf("You are player %d and you play with the %c \n", player, pieces);
    printf("Please wait until it is your turn to play, and then write the number of the tile you want to place your piece in. \n \n");
    printf("The tiles are numbered from the upper left corner to the lower right one as follows: up to down and left to right \n \n");
    printf("What is your name?");

    printf("\n Your name: ");
    bzero(buffer,256);
    fgets(buffer,255,stdin);

    n = write(sockfd,buffer,strlen(buffer));


    while( strstr(buffer, "Stop play" ) == 0 ) {
    
        bzero(buffer,256);
        n = read(sockfd,buffer,255);
        if (n < 0) 
            error("ERROR reading from socket");

        if(buffer[0] == '\0')
            break;
        else    
            show_board(buffer);

        printf("\n Please enter the move: ");
        bzero(buffer,256);
        fgets(buffer,255,stdin);

        n = write(sockfd,buffer,strlen(buffer));
        if (n < 0) 
            error("ERROR writing to socket");
    }

    printf("Game ended. Check the leaderboard! \n");
    close(sockfd);
    return 0;
}
