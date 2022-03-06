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

#define MYPORT "10000" // the port users will be connecting to
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

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
	{
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

void toUpperLine(char *str) {
	for (int i = 0; i < strlen(str); i++) {
		str[i] = toupper(str[i]);
	}
}

int main(int argc, char *argv[])
{

	int status;
	struct addrinfo hints, *res, *p;
	struct sigaction sa;
	struct sockaddr_storage their_addr;
	socklen_t addr_size;
	char s[INET6_ADDRSTRLEN];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // AF_INET or AF_INET6 to force version
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	if ((status = getaddrinfo(NULL, MYPORT, &hints, &res)) != 0)
	{
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(1);
	}

	int sock;

	for (p = res; p != NULL; p = p->ai_next)
	{
		sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
		if (sock < 1)
		{
			fprintf(stderr, "error in socket creation");
			continue;
		}

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
		fprintf(stderr, "server failed to bind");
		exit(1);
	}

	if (listen(sock, BACKLOG) == -1)
	{
		fprintf(stderr, "error in listening port");
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

	printf("server: waiting for connections...\n");

	while (1)
	{
		addr_size = sizeof their_addr;
		int new_fd;
		if ((new_fd = accept(sock, (struct sockaddr *)&their_addr, &addr_size)) == -1)
		{
			fprintf(stderr, "error in accepting connection");
			return 2;
		};

		inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
		printf("server: connection %s\n", s);

		if (!fork())
		{				 // this is the child process
			close(sock); // child doesn't need the listener
			if (send(new_fd, "Hello, Client!", 14, 0) == -1)
				perror("send");
			//
			int bytes;
			char buf[20];
			if ((bytes = recv(new_fd, buf, 19, 0)) == -1)
			{
				perror("recv");
			}

			buf[bytes] = '\0';

			printf("server: client's messsage '%s'\n", buf);
			toUpperLine(buf);
			
			if (send(new_fd, buf, 19, 0) == -1) {
				perror("send");
			}
			
			//
			close(new_fd);
			exit(0);
		}
		close(new_fd);
	}
	return 0;
}