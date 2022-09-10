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

#define port "53"
#define MAX_ADDRESS 50
// my dns server 192.168.131.227

// caching pakeitimas, saugoti serverius ip adresus ir jei serveris neveikia, naudoti cache'a

void getDomainAddress(char *_host, char *domainAddress);

int main(int argc, char *argv[])
{
    char *_host = argv[1];

    if (_host == NULL)
    {
        printf("Provide host as argument\n");
        return 1;
    }

    while (1)
    {
        char domain[100];
        printf("\nWrite your domain: ");
        scanf("%s", domain);
        getDomainAddress(_host, domain);
    }

    return 0;
}

void getDomainAddress(char *_host, char *domainAddress)
{
    int size = 0;

    unsigned char buf[1000000], *qname, *reader;
    int i, j, stop, s;

    struct sockaddr_in a;

    struct RES_RECORD answers[20], auth[20], addit[20]; // the replies from the DNS server
    struct sockaddr_in dest;

    struct DNS_HEADER *dns = NULL;
    struct QUESTION *qinfo = NULL;

    FILE *cache = fopen("cache.txt", "a+");

    // s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if ((s = connectToServer(_host, "53")) == -1)
    {
        char address[MAX_ADDRESS];
        char wordsList[MAX_ADDRESS][MAX_ADDRESS];
        int size = 0;
        char *token;
        while (fgets(address, MAX_ADDRESS, cache))
        {
            address[strlen(address) - 1] = '\0';
            token = strtok(address, " ");
            if (strcmp(token, domainAddress) == 0)
            {
                token = strtok(NULL, " ");
                printf("%s ip address: %s\n", address, token);
            }
            strcpy(wordsList[size], address);
            size++;
        }
        fclose(cache);
        return;
    }

    dest.sin_family = AF_INET;
    dest.sin_port = htons(53);               // 53 port
    dest.sin_addr.s_addr = inet_addr(_host); // dns server host

    dns = (struct DNS_HEADER *)&buf;

    dns->id = (unsigned short)htons(getpid());
    dns->qr = 0;
    dns->opcode = 0;
    dns->aa = 0;
    dns->tc = 0;
    dns->rd = 1;
    dns->ra = 0;
    dns->z = 0;
    dns->ad = 0;
    dns->cd = 0;
    dns->rcode = 0;
    dns->q_count = htons(1);
    dns->ans_count = 0;
    dns->auth_count = 0;
    dns->add_count = 0;

    qname = (unsigned char *)&buf[sizeof(struct DNS_HEADER)];

    configureDomain(qname, domainAddress);
    qinfo = (struct QUESTION *)&buf[sizeof(struct DNS_HEADER) + (strlen((const char *)qname) + 1)];

    qinfo->qtype = htons(1);  // type of the query , A , MX , CNAME , NS etc
    qinfo->qclass = htons(1); // its internet (lol)

    printf("Sending query...\n");
    if (send(s, (char *)buf, sizeof(struct DNS_HEADER) + (strlen((const char *)qname) + 1) + sizeof(struct QUESTION), 0) < 0)
    {
        perror("failed to send query");
    }
    // Receive the answer
    i = sizeof dest;
    if (recv(s, (char *)buf, 65536, 0) < 0)
    {
        perror("failed to get response");
    }

    dns = (struct DNS_HEADER *)buf;

    // move ahead of the dns header and the query field
    reader = &buf[sizeof(struct DNS_HEADER) + (strlen((const char *)qname) + 1) + sizeof(struct QUESTION)];

    unsigned char *name = readResponseBuffer(reader, buf, &size);
    reader = reader + size;
    struct R_DATA *data = (struct R_DATA *)(reader);
    reader = reader + sizeof(struct R_DATA);
    unsigned char *record;
    if (ntohs(data->type) == 1) // ipv4
    {
        record = (unsigned char *)malloc(ntohs(data->data_len));
        for (j = 0; j < ntohs(data->data_len); j++)
        {
            record[j] = reader[j];
        }
        record[ntohs(data->data_len)] = '\0';
    }
    else
    {
        record = readResponseBuffer(reader, buf, &size);
    }
    if (ntohs(data->type) == 1) // ipv4
    {
        printf("%s ", name);
        long *p;
        p = (long *)record;
        a.sin_addr.s_addr = (*p);
        printf("ip address: %s\n\n", inet_ntoa(a.sin_addr));
        fprintf(cache, "%s %s\n", name, inet_ntoa(a.sin_addr));
        fflush(cache);
    }
    else
    {
        printf("Couldn't find ip address\nChechking cache...\n");
    }
    fclose(cache);
    close(s);
}