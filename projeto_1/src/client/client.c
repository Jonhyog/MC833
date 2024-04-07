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

	if (argc < 2) {
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
	uint16_t response_buff[650000];
	char op[10];
	char clear_line[128];
	int len;

	if(argc>2 && strcmp(argv[2], "-adm")==0){
		printf("Seja bem vindo!\n\nOperações:\nAdd: Adicionar uma música\nRem: Remover uma música pelo ID\nList: Mostrar músicas\n\nPara sair digite exit\n");
	}
	else{
		printf("Seja bem vindo!\n\nOperações:\nList: Mostrar músicas\n\nMais operações disponíveis para administradores, adicione a flag -adm\n\nPara sair digite exit\n");
	}

	while (meta_hints.pkt_type != MUSIC_END){
		printf("Digite a operação: ");
		scanf(" %s", op);
		fgets(clear_line, 128, stdin);

		if(strcmp(op, "add") == 0 && argc>2 && strcmp(argv[2], "-adm")==0){
			meta.id = 0; // placeholder - true id decided by server

			//add musica
			printf("\n Título: ");
			fgets((char *) &meta.title, 128, stdin);
			meta.title[strcspn((char *) meta.title, "\n")] = 0;

			printf("\n Intérprete: ");
			fgets((char *) &meta.interpreter, 128, stdin);
			meta.interpreter[strcspn((char *) meta.interpreter, "\n")] = 0;

			printf("\n Idioma: ");
			fgets((char *) &meta.language, 128, stdin);
			meta.language[strcspn((char *) meta.language, "\n")] = 0;

			printf("\n Tipo de música: ");
			fgets((char *) &meta.category, 128, stdin);
			meta.category[strcspn((char *) meta.category, "\n")] = 0;

			printf("\n Refrão: ");
			fgets((char *) &meta.chorus, 128, stdin);
			meta.chorus[strcspn((char *) meta.chorus, "\n")] = 0;

			printf("\n Ano de Lançamento: ");
			scanf(" %d", &meta.release_year);

			meta_hints.pkt_filter = 0;
    		meta_hints.pkt_op = MUSIC_ADD;
    		meta_hints.pkt_numres = 1;
    		meta_hints.pkt_status = 0;
			buff = htonmm(&meta, &meta_hints);
    		len = (int) meta_hints.pkt_size;
    		sendall(sockfd, buff, &len);
    
			printf("waiting response\n");
			recvall(sockfd, response_buff, 650000, 0);
			ntohmm(response_buff, &meta_hints);

			if (meta_hints.pkt_type == MUSIC_RES) {
				printf("server responded op %d with status %d\n", meta_hints.pkt_op, meta_hints.pkt_status);
			}
		}

		 else if(strcmp(op, "rem") == 0 && argc>2 && strcmp(argv[2], "-adm")==0){
			printf("\n ID: ");
			scanf(" %d", &meta.id);

			meta_hints.pkt_filter = 0;
    		meta_hints.pkt_op = MUSIC_DEL;
    		meta_hints.pkt_numres = 1;
    		meta_hints.pkt_status = 0;

			buff = htonmm(&meta, &meta_hints);
    		len = (int) meta_hints.pkt_size;
    		sendall(sockfd, buff, &len);

			printf("waiting response\n");
			recvall(sockfd, response_buff, 2048, 0);
			ntohmm(response_buff, &meta_hints);

			if(meta_hints.pkt_status != MUSIC_OK){
				printf("\nNenhuma música com este id\n");
			}

			if (meta_hints.pkt_type == MUSIC_RES) {
				printf("server responded op %d with status %d\n", meta_hints.pkt_op, meta_hints.pkt_status);
			}
		}

		else if(strcmp(op, "list") == 0){
			char fields[2048];
			fgets((char *) fields, 2048, stdin);

			// char filter[8];
			MusicMeta *server_res;
			uint16_t filter = 0;
			int counter = 0;
			char **tokens = (char **) malloc(sizeof(char *) * 8);

			for (char *tok = strtok(fields, ";"); tok && *tok; tok = strtok(NULL, ";\n")) {
				tokens[counter] = malloc(sizeof(char) * 128);
				strcpy(tokens[counter], tok);
				counter++;
			}

			memset(&meta, 0, sizeof(MusicMeta));
			for (int i = 0; i < counter; i++) {
				char *info = strtok(tokens[i], "=");
				if(strcmp(info, "id") == 0){
					meta.id = atoi(strtok(NULL, "=\n"));
					filter |= (1 << 0);
				}
				if(strcmp(info, "year") == 0){
					meta.release_year = atoi(strtok(NULL, "=\n"));
					filter |= (1 << 1);
				}
				if(strcmp(info, "title") == 0){
					strcpy((char *) meta.title, strtok(NULL, "=\n"));
					filter |= (1 << 2);
				}
				if(strcmp(info, "interpreter") == 0){
					strcpy((char *) meta.interpreter, strtok(NULL, "=\n"));
					filter |= (1 << 3);
				}
				if(strcmp(info, "lang") == 0){
					strcpy((char *) meta.language, strtok(NULL, "=\n"));
					filter |= (1 << 4);
				}
				if(strcmp(info, "type") == 0){
					strcpy((char *) meta.category, strtok(NULL, "=\n"));
					filter |= (1 << 5);
				}
				if(strcmp(info, "chorus") == 0){
					strcpy((char *) meta.chorus, strtok(NULL, "=\n"));
					filter |= (1 << 6);
				}
        	}

			printf("searching with filter %d\n", filter);
			meta_hints.pkt_filter = filter;
			meta_hints.pkt_op = MUSIC_LIST;
    		meta_hints.pkt_numres = 1;
    		meta_hints.pkt_status = 0;
			
			buff = htonmm(&meta, &meta_hints);
    		len = (int) meta_hints.pkt_size;

    		sendall(sockfd, buff, &len);
			printf("waiting response\n");
			recvall(sockfd, response_buff, 2048, 0);
			server_res = ntohmm(response_buff, &meta_hints);
	
			if (meta_hints.pkt_type == MUSIC_RES) {
				printf("server responded op %d with status %d\n", meta_hints.pkt_op, meta_hints.pkt_status);
				printf("listing musics with matching fields\n");

				if(meta_hints.pkt_numres == 0){
					printf("\nNenhuma música com essas carcterísticas encontrada!\n");
				}

				for (int i = 0; i < meta_hints.pkt_numres; i++) {
					printf("\t%d, %d, %s, %s, %s, %s, %s\n",
						server_res[i].id,
						server_res[i].release_year,
						server_res[i].title,
						server_res[i].interpreter,
						server_res[i].language,
						server_res[i].category,
						server_res[i].chorus
            		);
				}
			}
		}
		else if(strcmp(op, "exit") == 0){
			char confirm[3];
			printf("Deseja sair? (y/n): ");
			scanf("%s", confirm);
			if(strcmp(confirm, "y") == 0){
				meta_hints.pkt_op = MUSIC_CLOSE;
    			meta_hints.pkt_numres = 1;
    			meta_hints.pkt_status = 0;
				meta_hints.pkt_type = MUSIC_END;

				buff = htonmm(&meta, &meta_hints);
				len = (int) meta_hints.pkt_size;
				sendall(sockfd, buff, &len);

				printf("waiting response\n");
				recvall(sockfd, response_buff, 2048, 0);
				ntohmm(response_buff, &meta_hints);
				printf("server responded op %d with status %d\n", meta_hints.pkt_op, meta_hints.pkt_status);

				if (meta_hints.pkt_status != MUSIC_OK) {
					meta_hints.pkt_type = MUSIC_ADD; // will be overwritten in next iter
				}
			}
		}
		else{
			printf("Operação não suportada ou reservada para administradores, tente novamente\n");
		}
	}

	close(sockfd);

	return 0;
}
