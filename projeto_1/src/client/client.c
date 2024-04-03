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

// add x
// rem x
// list year=x
// list lang=x
// list type=x
// list id=x
// list

	while(meta_hints.pkt_type != EXIT){
		char *entrada;
		scanf("%s", entrada);

		op = strtok(entrada, " ");
		if(strcmp(op, "add") == 0){
			meta.id = strtok(NULL, " ");
			//add musica
			printf("\n Título: ");
			scanf("%s", meta.title);
			printf("\n Intérprete: ");
			scanf("%s", meta.interpreter);
			printf("\n Idioma: ");
			scanf("%s", meta.language);
			printf("\n Tipo de música: ");
			scanf("%s", meta.category);
			printf("\n Refrão: ");
			scanf("%s", meta.chorus);
			printf("\n Ano de Lançamento: ");
			scanf("%s", meta.release_year);

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

			printf("waiting response\n");

			uint16_t response_buff[2048];

			recvall(sockfd, response_buff, 2048, 0);

			// MusicMeta *server_res;

			ntohmm(response_buff, &meta_hints);

			if (meta_hints.pkt_type == MUSIC_RES) {
				printf("server responded op %d with status %d\n", meta_hints.pkt_op, meta_hints.pkt_status);
			}
		}

		if(strcmp(op, "rem") == 0){
			meta.id = strtok(NULL, " ");
			meta_hints.pkt_filter = 0;
    		meta_hints.pkt_op = MUSIC_DEL;
    		meta_hints.pkt_numres = 1;
    		meta_hints.pkt_status = 0;

			buff = htonmm(&meta, &meta_hints);

    		printf("META SIZE: %d\n", meta_hints.pkt_size);

    		len = (int) meta_hints.pkt_size;
    		sendall(sockfd, buff, &len);

			printf("waiting response\n");

			recvall(sockfd, response_buff, 2048, 0);

			// MusicMeta *server_res;

			ntohmm(response_buff, &meta_hints);

			if (meta_hints.pkt_type == MUSIC_RES) {
				printf("server responded op %d with status %d\n", meta_hints.pkt_op, meta_hints.pkt_status);
			}
		}

		//tok vai ser do tipo id=x lan=y
		if(strcmp(op, "list") == 0){
			char filter[8];
			for (char *tok = strtok(entrada, " "); tok && *tok; tok = strtok(NULL, " ")) {

				char *info = strtok(tok, "=");
				if(strcmp(info,"id")){
					meta.id = strtok(NULL, "=");
					filter[8] = '1';
				}
				if(strcmp(info,"year")){
					meta.release_year = strtok(NULL, "=");
					filter[7] = '1';
				}
				if(strcmp(info,"title")){
					strcpy(meta.title, strtok(NULL, "="));
					filter[6] = '1';
				}
				if(strcmp(info,"interpreter")){
					strcpy(meta.interpreter, strtok(NULL, "="));
					filter[5] = '1';
				}
				if(strcmp(info,"lang")){
					strcpy(meta.language, strtok(NULL, "="));
					filter[4] = '1';
				}
				if(strcmp(info,"type")){
					strcpy(meta.category, strtok(NULL, "="));
					filter[3] = '1';
				}
				
        	}
			meta_hints.pkt_filter = strtoul (filter, NULL, 0);
			meta_hints.pkt_op = MUSIC_LIST;
    		meta_hints.pkt_numres = 1;
    		meta_hints.pkt_status = 0;

			buff = htonmm(&meta, &meta_hints);

    		printf("META SIZE: %d\n", meta_hints.pkt_size);

    		len = (int) meta_hints.pkt_size;
    		sendall(sockfd, buff, &len);

			printf("waiting response\n");

			recvall(sockfd, response_buff, 2048, 0);
			ntohmm(response_buff, &meta_hints);
	
			if (meta_hints.pkt_type == MUSIC_RES) {
				printf("server responded op %d with status %d\n", meta_hints.pkt_op, meta_hints.pkt_status);
			}
		}
	}
	close(sockfd);

	return 0;



	

	
}
