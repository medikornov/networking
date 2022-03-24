#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>
#include "utils.c"

#define BUFFSIZE 99
#define STRING_LIST 99

int main(int argc, char *argv[])
{
    char *host = argv[1];
    char *port = argv[2];

    if (host == NULL || port == NULL)
    {
        printf("Provide host and port as arguments\n");
        return 1;
    }

    int listening_sock;
    if ((listening_sock = createSocket(host, port)) == -1)
    {
        printf("Error in creating socket");
        return 1;
    };

    char names [STRING_LIST][STRING_LIST];
    // 0 - empty, 1 - finished, 2 - name needed
    int states [STRING_LIST] = {0};
    states[listening_sock] = 1;

    struct sockaddr_storage connected_addr;
    socklen_t addr_size;
    addr_size = sizeof connected_addr;
    char s[INET6_ADDRSTRLEN];

    fd_set master;
    fd_set readfds;
    int fds_count = listening_sock;
    FD_ZERO(&master);
    FD_ZERO(&readfds);

    FD_SET(listening_sock, &master);
    states[listening_sock] = 1;

    int bytes;
    char buf[BUFFSIZE];

    while (1)
    {
        readfds = master;
        if (select(fds_count + 1, &readfds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        for (int i = 0; i <= fds_count; i++) {
            if (FD_ISSET(i, &readfds)) {
                if (i == listening_sock) {
                    // new connection
                    int new_conn;
                    if ((new_conn = accept(listening_sock, (struct sockaddr *)&connected_addr, &addr_size)) == -1) {
                        fprintf(stderr, "error in accepting connection\n");
                        continue;
                    }
                    FD_SET(new_conn, &master);
                    states[new_conn] = 2;
                    if (new_conn > fds_count)
                        fds_count = new_conn;
                    printf("New connection: socket %d\n", new_conn);
                    char *send_name = "ATSIUSKVARDA\n\0";
                    if (send(new_conn, send_name, strlen(send_name), 0) == -1) {
                        perror("send");
                    }
                }
                else {
                    int bytes;
                    if ((bytes = recv(i, buf, BUFFSIZE, 0)) == -1) {
                        // recv error
                        perror("recv");
                        close(i);
                        FD_CLR(i, &master);
                        states[i] = 0;
                    }
                    else if (bytes == 0) {
                        // connection closed
                        printf("Connection closed: socket %d\n", i);
                        close(i);
                        FD_CLR(i, &master);
                        states[i] = 0;
                        addStringToList(names, i, "               \0", 15);
                    }
                    else {
                        // got message from client
                        //buf[bytes] = '\0';
                        //printf("dydys buf %zu, buf %s", strlen(buf), buf);
                        buf[bytes - 2] = '\0';
                        if (states[i] == 2) {
                            if (searchForStringInList(names, STRING_LIST - 1, buf) == 0) {
                                addStringToList(names, i, buf, bytes);
                                strcat(buf, " joined the chat\n\0");
                                for (int j = 0; j <= fds_count; j++) {
                                    if (FD_ISSET(j, &master)) {
                                        if (j == i) {
                                            char *name_ok = "VARDASOK\n\0";
                                            if (send(j, name_ok, strlen(name_ok), 0) == -1) {
                                                perror("send");
                                            }
                                        }
                                        else if (j == listening_sock) {
                                            printf("%s", buf);
                                        }
                                    }
                                }
                                states[i] = 1;
                            }
                            else {
                                printf("there is already that name\n");
                            }
                        }
                        else {
                            for (int j = 0; j <= fds_count; j++) {
                                if (FD_ISSET(j, &master)) {
                                    if (j == listening_sock) {
                                        printf("PRANESIMAS %s: %s\n", names[i], buf);
                                    }
                                    char message[BUFFSIZE] = "PRANESIMAS";
                                    strcat(message, names[i]);
                                    strcat(message, ": ");
                                    strcat(message, buf);
                                    strcat(message, "\n\0");
                                    if (j != listening_sock) {
                                        if (send(j, message, strlen(message), 0) == -1) {
                                            perror("send");
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    close(listening_sock);

    return 0;
}