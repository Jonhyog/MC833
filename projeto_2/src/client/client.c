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

int create_socket(char *addr, char *port, struct addrinfo hints, struct addrinfo **p)
{
	int sockfd, rv;
	struct addrinfo *servinfo, *p_temp;

	if ((rv = getaddrinfo(addr, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and connect to the first we can
	for(p_temp = servinfo; p_temp != NULL; p_temp = p_temp->ai_next) {
		if ((sockfd = socket(p_temp->ai_family, p_temp->ai_socktype,
				p_temp->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p_temp->ai_addr, p_temp->ai_addrlen) == -1) {
			perror("client: connect");
			close(sockfd);
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (p_temp == NULL) {
		fprintf(stderr, "client: failed to connect\n");
		return 2;
	}

	*(p) = p_temp;
	return sockfd;

}

void greet_prompt(int auth)
{
	if (auth)
		printf("Seja bem vindo!\n\nOperações:\nAdd: Adicionar uma música\nRem: Remover uma música pelo ID\nList: Mostrar músicas\n\nPara sair digite exit\n");
	else
		printf("Seja bem vindo!\n\nOperações:\nList: Mostrar músicas\n\nMais operações disponíveis para administradores, adicione a flag -adm\n\nPara sair digite exit\n");
}

void handle_add(int auth, MusicMeta *mm, MMHints *hints)
{
	if (!auth) {
		printf("Você não tem permissão para realizar essa operação!");
		return;
	}

	// Gets Music MetaData
	// Placeholder - ID is given by server
	mm->id = 0; 

	printf("\n Título: ");
	fgets((char *) &mm->title, 128, stdin);
	mm->title[strcspn((char *) mm->title, "\n")] = 0;

	printf("\n Intérprete: ");
	fgets((char *) &mm->interpreter, 128, stdin);
	mm->interpreter[strcspn((char *) mm->interpreter, "\n")] = 0;

	printf("\n Idioma: ");
	fgets((char *) &mm->language, 128, stdin);
	mm->language[strcspn((char *) mm->language, "\n")] = 0;

	printf("\n Tipo de música: ");
	fgets((char *) &mm->category, 128, stdin);
	mm->category[strcspn((char *) mm->category, "\n")] = 0;

	printf("\n Refrão: ");
	fgets((char *) &mm->chorus, 128, stdin);
	mm->chorus[strcspn((char *) mm->chorus, "\n")] = 0;

	printf("\n Ano de Lançamento: ");
	scanf(" %d", &mm->release_year);

	// Sets up hints for pkt
	hints->pkt_filter = 0;
	hints->pkt_op = MUSIC_ADD;
	hints->pkt_numres = 1;
	hints->pkt_status = 0;
}

void handle_remove(int auth, MusicMeta *mm, MMHints *hints)
{
	if (!auth) {
		printf("Você não tem permissão para realizar essa operação!");
		return;
	}

	// Gets ID MetaData
	printf("\n ID: ");
	scanf(" %d", &mm->id);

	hints->pkt_filter = 0;
	hints->pkt_op = MUSIC_DEL;
	hints->pkt_numres = 1;
	hints->pkt_status = 0;
}

void handle_list(int auth, MusicMeta *mm, MMHints *hints)
{
	if (!auth) {
		printf("Você não tem permissão para realizar essa operação!");
		return;
	}
}

void handle_download(int auth, MusicMeta *mm, MMHints *hints)
{
	if (!auth) {
		printf("Você não tem permissão para realizar essa operação!");
		return;
	}
}

void handle_exit(int auth, MusicMeta *mm, MMHints *hints)
{
	char confirm[3];

	if (!auth) {
		printf("Você não tem permissão para realizar essa operação!");
		return;
	}

	// Exit Confirmation
	printf("Deseja sair? (y/n): ");
	scanf("%s", confirm);

	// Sets up exit pkt
	if(strcmp(confirm, "y") == 0){
		hints->pkt_op = MUSIC_CLOSE;
		hints->pkt_numres = 1;
		hints->pkt_status = 0;
		hints->pkt_type = MUSIC_END;
	}
}

void handle_operation(char *op, int auth)
{
	int i;
	char operations[] = {"add", "rem", "list", "download", "exit"};
	MusicMeta meta;
    MMHints  meta_hints;

	for (i = 0; i < 5; i++) {
		if (strcmp(op, operations[i]) == 0) break;
	}

	switch (i)
	{
	case 0:
		handle_add(auth, &meta, &meta_hints);
		break;
	case 1:
		handle_remove(auth, &meta, &meta_hints);
		break;
	case 2:
		handle_list(1, &meta, &meta_hints);
		break;
	case 3:
		handle_download(1, &meta, &meta_hints);
		break;
	case 4:
		handle_exit(1, &meta, &meta_hints);
		break;
	default:
		printf("Operação não suportada. Tente novamente\n");
		return;
	}

	// Generates and sends pkt

}

int main(int argc, char *argv[])
{
	int sockfd, downloadfd;  
	struct addrinfo hints, *p;
    MusicMeta meta;
    MMHints  meta_hints;
    uint16_t *buff;
	uint16_t response_buff[650000];
	char op[10];
	char clear_line[128];
	int len;
	int auth, state = 0; // Waiting Input == 0 : Waiting Response = 1

	if (argc < 2) {
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

	// Creates TCP Socket
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	sockfd = create_socket(argv[1], PORT, hints, &p);

	// FIX-ME: USE IPv6
	// Creates UDP Socket
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	downloadfd = create_socket(argv[1], PORT, hints, &p);

	// Handles user input
	printf("%d\n", state);
	auth = argc > 2 && strcmp(argv[2], "-adm") == 0 ? 1 : 0;
	greet_prompt(auth);

	while (meta_hints.pkt_type != MUSIC_END){
		if (state == 0) {
			state = 1;

			printf("Digite a operação: ");
			scanf(" %s", op);
			fgets(clear_line, 128, stdin);

			handle_operation(op, auth);

			// handle input
		} else {
			// await responde
		}
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
			printf("Os parâmtros disponíveis para filtrar são: id, year, title, interpreter, lang, type, chorus\nDigite aqueles que desejar da forma 'campo=valor', separando-os com ';'\n");
			fgets((char *) fields, 2048, stdin);

			MusicMeta *server_res;
			uint16_t filter = 0;
			int counter = 0;
			int not_identified = 0;
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
				else if(strcmp(info, "year") == 0){
					meta.release_year = atoi(strtok(NULL, "=\n"));
					filter |= (1 << 1);
				}
				else if(strcmp(info, "title") == 0){
					strcpy((char *) meta.title, strtok(NULL, "=\n"));
					filter |= (1 << 2);
				}
				else if(strcmp(info, "interpreter") == 0){
					strcpy((char *) meta.interpreter, strtok(NULL, "=\n"));
					filter |= (1 << 3);
				}
				else if(strcmp(info, "lang") == 0){
					strcpy((char *) meta.language, strtok(NULL, "=\n"));
					filter |= (1 << 4);
				}
				else if(strcmp(info, "type") == 0){
					strcpy((char *) meta.category, strtok(NULL, "=\n"));
					filter |= (1 << 5);
				}
				else if(strcmp(info, "chorus") == 0){
					strcpy((char *) meta.chorus, strtok(NULL, "=\n"));
					filter |= (1 << 6);
				}
				else if (strcmp(info, "\n") == 0) {
					
				}
				else{
					not_identified = 1;
					printf("Parâmetro '%s' não identificado.\n", info);
				}
        	}

			if (!not_identified) {
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
		}
		else if (strcmp(op, "download") == 0) {
			printf("ID: ");
			scanf(" %d", &meta.id);

			meta_hints.pkt_filter = 0;
    		meta_hints.pkt_op = MUSIC_GET;
    		meta_hints.pkt_numres = 1;
    		meta_hints.pkt_status = 0;

			buff = htonmm(&meta, &meta_hints);
    		len = (int) meta_hints.pkt_size;

			if (sendto(downloadfd, buff, len, 0, p->ai_addr, p->ai_addrlen) == -1) {
				perror("failed to send udp pkt");
				exit(1);
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
