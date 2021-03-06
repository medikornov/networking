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
#define RESULTS_FILE "results.txt"

int main(int argc, char *argv[])
{
	char *host = argv[1];
	char *port = argv[2];

	char *client_host = argv[3];
	char *client_port = argv[4];

	if (host == NULL || port == NULL || client_host == NULL || client_port == NULL) {
		printf("Provide host and port as arguments\n");
		return 1;
	}

	FILE *results_file = fopen(RESULTS_FILE, "w");
    if (results_file == NULL) {
        printf("Couldnt open file");
        return 1;
    }

	int sock;
	if ((sock = createSocket(host, port)) == -1) {
		printf("Error in creating socket");
        return 1;
	};
	int client_conn;
	if ((client_conn = connectToServer(client_host, client_port)) == -1) {
        printf("Error in connecting to server");
        return 1;
    }

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
			break;
		}
		buf[bytes] = '\0';
		printf("Client: %s", buf);

		if (send(client_conn, buf, bytes + 1, 0) == -1)
			perror("send");

		if ((bytes = recv(client_conn, buf, BUFFSIZE, 0)) == -1) {
            perror("recv");
        }
		buf[bytes] = '\0';
		printf("Server: %s", buf);
		fprintf(results_file, "%s", buf);
		if (send(new_fd, buf, bytes + 1, 0) == -1)
			perror("send");
	}
	close(sock);
	close(new_fd);
	return 0;
}