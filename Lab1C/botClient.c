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
#define FILE_NAME "words.txt"
#define BOT_NICKNAME "VERYSECRETBOT  "
#define NAME_SIZE 20
#define MESSAGE_SIZE 1000

void getNameAndMessage(char* message, int size, char* return_name, char* return_message) {
    int collonPlace = 0;
    for (int i = 9; i < size; i++) {
        if (message[i] == ':') {
            collonPlace = i;
            break;
        }
    }
    strncpy(return_name, message + 10, collonPlace - 10);
    strncpy(return_message, message + collonPlace + 2, size - 1);
}

int checkIfMessageContainsTrackableWords(char words_list[][MAXDATASIZE], int size_of_list, char* message) {
    int init_size = strlen(message);
	char delim[] = " ";
	char *ptr = strtok(message, delim);

	while (ptr != NULL) {
        for (int i = 0; i < size_of_list; ++i) {
            if(strcmp(words_list[i], ptr) == 0) {
                return 1;
            }
        }
		ptr = strtok(NULL, delim);
	}
    return 0;
}

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

    char word[MAXDATASIZE];
    char wordsList[MAXDATASIZE][MAXDATASIZE];
    int size = 0;
    while (fgets(word, MAXDATASIZE, data_file)) {
        word[strlen(word) - 1] = '\0';
        strcpy(wordsList[size], word);
        size++;
    }
    printf("dangerous words: \n");
    for (int i = 0; i < size; i++) {
        printf("%s\n", wordsList[i]);
    }

    // connecting to server
    int sockfd;
    if ((sockfd = connectToServer(host, port)) == -1) {
        printf("Error in connecting to server");
        return 1;
    }

    char buf[MAXDATASIZE];
    while (1) {
        int bytes;
        if ((bytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) != -1) {
            buf[bytes - 1] = '\0';
            if (strcmp(buf, "ATSIUSKVARDA") == 0) {
                if(send(sockfd, BOT_NICKNAME, strlen(BOT_NICKNAME), 0) == -1) {
                    perror("error in sending bot nickname");
                } 
            }
            else {
                char name[NAME_SIZE] = {'\0'};
                char message[MESSAGE_SIZE] = {'\0'};
                getNameAndMessage(buf, bytes, name, message);
                //printf("%s: %s\n", name, message);
                if (checkIfMessageContainsTrackableWords(wordsList, size, message)) {
                    char prepareMessage[NAME_SIZE + 10] = {'\0'};
                    strcat(prepareMessage, "#");
                    strcat(prepareMessage, name);
                    strcat(prepareMessage, "!  \0");
                    if(send(sockfd, prepareMessage, strlen(prepareMessage), 0) == -1) {
                        perror("error in sending bot nickname");
                    } 
                }
            }
        }
    }
    close(sockfd);

    return 0;
}
