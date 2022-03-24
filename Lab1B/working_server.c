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

#define BACKLOG 1
#define BUFFSIZE 1024

int main(int argc, char *argv[])
{
	struct addrinfo hints, *res, *p;
	struct sigaction sa;
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	int status;
	if ((status = getaddrinfo(argv[1], argv[2], &hints, &res)) != 0)
	{
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}

	int sock;
	for (p = res; p != NULL; p = p->ai_next)
	{
		char s[100];
		inet_ntop(p->ai_family, get_in_addr(p->ai_addr), s, sizeof s);
		fprintf(stderr, "Result %s\n", s);

		sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sock < 1)
		{
			fprintf(stderr, "error in socket creation\n");
			continue;
		}

		int yes = 1;
		if (bind(sock, res->ai_addr, res->ai_addrlen) == -1)
		{
			fprintf(stderr, "error in binding\n");
			continue;
		};
		break;
	}
	freeaddrinfo(res);

	if (p == NULL)
	{
		fprintf(stderr, "server failed to bind\n");
		exit(1);
	}

	if (listen(sock, BACKLOG) == -1)
	{
		fprintf(stderr, "error in listening port\n");
		exit(1);
	};

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1)
	{
		perror("sigaction");
		exit(1);
	}

	printf("Server: waiting for connection...\n");

	addr_size = sizeof their_addr;
	int new_fd;
	if ((new_fd = accept(sock, (struct sockaddr *)&their_addr, &addr_size)) == -1)
	{
		fprintf(stderr, "error in accepting connection");
		return 2;
	};

	while (1)
	{
		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
		printf("Server: connection %s\n", s);

		int bytes;
		char buf[BUFFSIZE];
		if ((bytes = recv(new_fd, buf, BUFFSIZE, 0)) == -1)
			perror("recv");

		buf[bytes] = '\0';

		printf("server: client's messsage '%s'\n", buf);
		toUpperLine(buf);

		if (send(new_fd, buf, bytes + 1, 0) == -1)
			perror("send");

		close(new_fd);
		exit(0);
	}
	return 0;
}