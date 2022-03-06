#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>


#define MYPORT "10000" // the port users will be connecting to
#define MAXDATASIZE 100

// ntop - network to presentation ## inet_ntop
// pton - presentation to network ## inet_pton
// AF_INET - address family

void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int status;
    struct addrinfo hints, *res, *p;

    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    char buf[MAXDATASIZE];
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(NULL, MYPORT, &hints, &res)) != 0)
    {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return 1;
    }

    int sockfd;
    for (p = res; p != NULL; p = p->ai_next)
    {

        if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1)
        {
            perror("client socket");
            continue;
        };

        if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("client connect");
            continue;
        };
        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("client: connecting to %s\n", s);
    freeaddrinfo(res);

    int bytes;
    if ((bytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
        perror("recv");
        exit(1);
    }

    buf[bytes] = '\0';

    printf("client: server's messsage '%s'\n", buf);

    char msg[20];
    printf("Your message to server: ");
    scanf("%[^\n]s", msg); 
    if (send(sockfd, msg, strlen(msg), 0) == -1) {
        perror("sending message");
    }; 

    if ((bytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) == -1) {
        perror("recv");
        exit(1);
    }

    buf[bytes] = '\0';
    printf("Response: '%s'\n", buf);


    close(sockfd);
    

    return 0;
}