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

struct DNS_HEADER
{
    unsigned short id; // identification number
 
    unsigned char rd :1; // recursion desired
    unsigned char tc :1; // truncated message
    unsigned char aa :1; // authoritive answer
    unsigned char opcode :4; // purpose of message
    unsigned char qr :1; // query/response flag
 
    unsigned char rcode :4; // response code
    unsigned char cd :1; // checking disabled
    unsigned char ad :1; // authenticated data
    unsigned char z :1; // its z! reserved
    unsigned char ra :1; // recursion available
 
    unsigned short q_count; // number of question entries
    unsigned short ans_count; // number of answer entries
    unsigned short auth_count; // number of authority entries
    unsigned short add_count; // number of resource entries
};
 
//Constant sized fields of query structure
struct QUESTION
{
    unsigned short qtype;
    unsigned short qclass;
};
 
//Constant sized fields of the resource record structure
#pragma pack(push, 1)
struct R_DATA
{
    unsigned short type;
    unsigned short _class;
    unsigned int ttl;
    unsigned short data_len;
};
#pragma pack(pop)
 
//Pointers to resource record contents
struct RES_RECORD
{
    unsigned char *name;
    struct R_DATA *resource;
    unsigned char *rdata;
};
 
typedef struct
{
    unsigned char *name;
    struct QUESTION *ques;
} QUERY;

void configureDomain(unsigned char *dns, unsigned char *_host)
{
    int lock = 0;
    strcat((char *)_host, ".");
    for (int i = 0; i < strlen((char *)_host); i++)
    {
        if (_host[i] == '.')
        {
            *dns++ = i - lock;
            for (; lock < i; lock++)
            {
                *dns++ = _host[lock];
            }
            lock++;
        }
    }
    *dns++ = '\0';
}

unsigned char *readResponseBuffer(unsigned char *reader, unsigned char *buffer, int *count)
{
    unsigned char *name;
    unsigned int p = 0, jumped = 0, offset;
    int i, j;

    *count = 1;
    name = (unsigned char *)malloc(256);

    name[0] = '\0';

    // read the names in 3www6google3com format
    while (*reader != 0)
    {
        if (*reader >= 192)
        {
            offset = (*reader) * 256 + *(reader + 1) - 49152;
            reader = buffer + offset - 1;
            jumped = 1;
        }
        else
        {
            name[p++] = *reader;
        }

        reader = reader + 1;

        if (jumped == 0)
        {
            *count = *count + 1;
        }
    }

    name[p] = '\0';
    if (jumped == 1)
    {
        *count = *count + 1;
    }

    // now convert 3www6google3com0 to www.google.com
    for (i = 0; i < (int)strlen((const char *)name); i++)
    {
        p = name[i];
        for (j = 0; j < (int)p; j++)
        {
            name[i] = name[i + 1];
            i = i + 1;
        }
        name[i] = '.';
    }
    name[i - 1] = '\0'; // remove the last dot
    return name;
}

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
	hints.ai_family = AF_INET; // AF_INET or AF_INET6 to force version
	hints.ai_socktype = SOCK_DGRAM;
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
    hints.ai_family = AF_INET; // AF_INET or AF_INET6 to force version
    hints.ai_socktype = SOCK_DGRAM;
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