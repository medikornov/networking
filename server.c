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

#define BUFFSIZE 1024

int main(int argc, char *argv[])
{
	char *host = argv[1];
	char *port = argv[2];

	if (host == NULL || port == NULL) {
		printf("Provide host and port as arguments\n");
		return 1;
	}

	int sock;
	if ((sock = createSocket(host, port)) == -1) {
		printf("Error in creating socket");
        return 1;
	};

	struct sockaddr_storage connected_addr;
	socklen_t addr_size;
	addr_size = sizeof connected_addr;
	char s[INET6_ADDRSTRLEN];

	int new_fd;
	if ((new_fd = accept(sock, (struct sockaddr *)&connected_addr, &addr_size)) == -1) {
		fprintf(stderr, "error in accepting connection\n");
		return 2;
	};

	inet_ntop(connected_addr.ss_family, get_in_addr((struct sockaddr *)&connected_addr), s, sizeof s);
	printf("Server: connection %s\n", s);

	int bytes;
	char buf[BUFFSIZE];
	while (1) {
		if ((bytes = recv(new_fd, buf, BUFFSIZE, 0)) == -1)
			perror("recv");

		if (bytes == 0) {
			printf("Server closed\n");
			return 0;
		}
		buf[bytes] = '\0';

		printf("T1 server client: %s", buf);
		toUpperLine(buf);

		if (send(new_fd, buf, bytes + 1, 0) == -1)
			perror("send");
	}
	close(sock);
	close(new_fd);
	return 0;
}