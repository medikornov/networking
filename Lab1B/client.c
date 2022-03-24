#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include "utils.c"

#define MAXDATASIZE 1024
#define FILE_NAME "data.txt"

int main(int argc, char *argv[])
{
    char *host = argv[1];
    char *port = argv[2];

    if (host == NULL || port == NULL) {
        printf("Provide host and port as arguments\n");
        return 1;
    }

    FILE *data_file = fopen(FILE_NAME, "r");
    if (data_file == NULL) {
        printf("Couldnt open file");
        return 1;
    }

    char buf[MAXDATASIZE];
    int sockfd;
    if ((sockfd = connectToServer(host, port)) == -1) {
        printf("Error in connecting to server");
        return 1;
    }

    int bytes;
    char msg[MAXDATASIZE];
    while (fgets(msg, MAXDATASIZE, data_file)) {
        //printf("Your message to server: ");
        if(strlen(msg) == 0){
            printf("EOF\n");
            break;
        }

        if (send(sockfd, msg, strlen(msg), 0) == -1)
            perror("sending message");

        if ((bytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
            perror("recv");
            exit(1);
        }

        buf[bytes] = '\0';
        printf("Server: %s", buf);
    }
    close(sockfd);

    return 0;
}