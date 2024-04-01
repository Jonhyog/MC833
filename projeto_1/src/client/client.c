#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include "music.h"
#include "codec.h"

#define PORT "3490"

#define MAXDATASIZE 2048

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int sendall(int fd, uint16_t *buff, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len;  // how many we have left to send
    int n;

    printf("seding data:\n");
    while(total < *len) {
        n = send(fd, buff + total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
        printf("\t(%d/%d)\n", total, bytesleft);
    }
    printf("\t(%d/%d)\n", total, bytesleft);

    *len = total; // return number actually sent here

    printf("sent %d bytes!\n", total);

    return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
}


int main(int argc, char *argv[])
{
	int sockfd;  
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];

	if (argc != 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof s);
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

    MusicMeta meta;
    MMHints  meta_hints;
    uint16_t *buff;

    meta.id = 1;
    meta.release_year = 1723;
    strcpy((char *) meta.title, "As Quatro Estações (Le Quattro Stagioni)");
    strcpy((char *) meta.interpreter, "Antônio Vivaldi");
    strcpy((char *) meta.language, "Italiano");
    strcpy((char *) meta.category, "Clássica");
    strcpy((char *) meta.chorus, "N/A");

    meta_hints.pkt_filter = 0;
    meta_hints.pkt_op = MUSIC_ADD;
    meta_hints.pkt_numres = 1;
    meta_hints.pkt_status = 0;

    buff = htonmm(&meta, &meta_hints);

    printf("META SIZE: %d\n", meta_hints.pkt_size);

    int len = (int) meta_hints.pkt_size;
    sendall(sockfd, buff, &len);
    
    FILE *write_ptr;
    write_ptr = fopen("client_dump.bin", "wb");
    fwrite(buff, meta_hints.pkt_size, 1, write_ptr);

	close(sockfd);

	return 0;
}
