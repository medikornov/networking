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

#define BACKLOG 1
// ntop - network to presentation ## inet_ntop
// pton - presentation to network ## inet_pton
// AF_INET - address family
void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while (waitpid(-1, NULL, WNOHANG) > 0)
        ;
    errno = saved_errno;
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

void toUpperLine(char *str) {
    for (int i = 0; i < strlen(str); i++) {
        str[i] = toupper(str[i]);
    }
}

int createSocket(char *host, char *port) {
    struct addrinfo hints, *res, *p;
	struct sigaction sa;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int status;
	if ((status = getaddrinfo(host, port, &hints, &res)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		return -1;
	}

	int sock;
	for (p = res; p != NULL; p = p->ai_next) {
		char s[100];
		inet_ntop(p->ai_family, get_in_addr(p->ai_addr), s, sizeof s);
		fprintf(stderr, "Result %s\n", s);

		sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sock < 1) {
			fprintf(stderr, "error in socket creation\n");
			continue;
		}

		if (bind(sock, res->ai_addr, res->ai_addrlen) == -1) {
			fprintf(stderr, "error in binding\n");
			continue;
		};
		break;
	}
	freeaddrinfo(res);

	if (p == NULL) {
		fprintf(stderr, "couldn't find host or port\n");
		return -1;
	}

	if (listen(sock, BACKLOG) == -1) {
		fprintf(stderr, "error in listening port\n");
		return -1;
	};

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction\n");
		return -1;
	}

	printf("Server: waiting for connection...\n");
    return sock;
}

int connectToServer(char *host, char *port) {
    int status;
    struct addrinfo hints, *res, *p;

    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((status = getaddrinfo(host, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        return -1;
    }

    int sockfd;
    for (p = res; p != NULL; p = p->ai_next) {

        if ((sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1) {
            perror("client socket");
            continue;
        };

        if (connect(sockfd, res->ai_addr, res->ai_addrlen) == -1) {
            close(sockfd);
            perror("client connect");
            continue;
        };
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "couldn't find host or port\n");
        return -1;
    }
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    printf("Connecting to %s\n\n", s);
    freeaddrinfo(res);

    return sockfd;
}

int searchForStringInList(char list[][99], int size, char *str) {
    for (int i = 0; i < size; i++) {
        if (strcmp(list[i], str) == 0) {
            return 1;
        }
    }
    return 0;
}

void addStringToList(char list[][99], int place, char *str, int size_of_str) {
    for (int i = 0; i < size_of_str; i++) {
        list[place][i] = str[i];
    }
    list[place][size_of_str] = '\0';
}

void convertStringToNormal(char *str1, char *str2, int size) {
    for (int i = 0; i < size; i++) {
        printf("Symbol %d: %d\n", i, str1[i]);
    }
}