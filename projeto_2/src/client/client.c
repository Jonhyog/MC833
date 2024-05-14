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

int handle_add(int auth, MusicMeta *mm, MMHints *hints)
{
	if (!auth) {
		printf("Você não tem permissão para realizar essa operação!");
		return 1;
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

	return 0;
}

int handle_remove(int auth, MusicMeta *mm, MMHints *hints)
{
	if (!auth) {
		printf("Você não tem permissão para realizar essa operação!");
		return 1;
	}

	// Gets ID MetaData
	printf("\n ID: ");
	scanf(" %d", &mm->id);

	hints->pkt_filter = 0;
	hints->pkt_op = MUSIC_DEL;
	hints->pkt_numres = 1;
	hints->pkt_status = 0;

	return 0;
}

int handle_list(int auth, MusicMeta *mm, MMHints *hints)
{
	uint16_t filter = 0;
	int counter = 0, invalid_token = 0;
	char fields[2048];
	char tokens[8][256];

	if (!auth) {
		printf("Você não tem permissão para realizar essa operação!");
		return 1;
	}

	// Gets unparsed filter params
	printf("Os parâmtros disponíveis para filtrar são: id, year, title, interpreter, lang, type, chorus\nDigite aqueles que desejar da forma 'campo=valor', separando-os com ';'\n");
	fgets((char *) fields, 2048, stdin);

	// Tokenizes filter params
	for (char *tok = strtok(fields, ";"); tok && *tok; tok = strtok(NULL, ";\n")) {
		strcpy(tokens[counter++], tok);
	}
	
	// Calculates filter based on present tokens
	for (int i = 0; i < counter; i++) {
		char *info = strtok(tokens[i], "=");
		if(strcmp(info, "id") == 0){
			mm->id = atoi(strtok(NULL, "=\n"));
			filter |= (1 << 0);
		}
		else if(strcmp(info, "year") == 0){
			mm->release_year = atoi(strtok(NULL, "=\n"));
			filter |= (1 << 1);
		}
		else if(strcmp(info, "title") == 0){
			strcpy((char *) mm->title, strtok(NULL, "=\n"));
			filter |= (1 << 2);
		}
		else if(strcmp(info, "interpreter") == 0){
			strcpy((char *) mm->interpreter, strtok(NULL, "=\n"));
			filter |= (1 << 3);
		}
		else if(strcmp(info, "lang") == 0){
			strcpy((char *) mm->language, strtok(NULL, "=\n"));
			filter |= (1 << 4);
		}
		else if(strcmp(info, "type") == 0){
			strcpy((char *) mm->category, strtok(NULL, "=\n"));
			filter |= (1 << 5);
		}
		else if(strcmp(info, "chorus") == 0){
			strcpy((char *) mm->chorus, strtok(NULL, "=\n"));
			filter |= (1 << 6);
		}
		else if (strcmp(info, "\n") == 0) {
			
		}
		else{
			invalid_token = 1;
			printf("Parâmetro '%s' não identificado.\n", info);
		}
	}

	// Sets up pkt hints
	hints->pkt_filter = filter;
	hints->pkt_op = MUSIC_LIST;
	hints->pkt_numres = 1;
	hints->pkt_status = 0;

	return invalid_token;
}

int handle_download(int auth, MusicMeta *mm, MMHints *hints)
{
	if (!auth) {
		printf("Você não tem permissão para realizar essa operação!");
		return 1;
	}

	printf("ID: ");
	scanf(" %d", &mm->id);

	hints->pkt_filter = 0;
	hints->pkt_op = MUSIC_GET;
	hints->pkt_numres = 1;
	hints->pkt_status = 0;

	return 0;
}

int handle_exit(int auth, MusicMeta *mm, MMHints *hints)
{
	char confirm[3];

	if (!auth) {
		printf("Você não tem permissão para realizar essa operação!");
		return 1;
	}

	// Exit Confirmation
	printf("Deseja sair? (y/n): ");
	scanf("%s", confirm);

	// Sets up exit pkt
	hints->pkt_op = MUSIC_CLOSE;
	hints->pkt_numres = 1;
	hints->pkt_status = 0;
	hints->pkt_type = MUSIC_END;

	if(strcmp(confirm, "n") == 0) return 1;
	return 0;
}

MusicMeta * talk_tcp(MusicMeta *mm, MMHints *hints, int fd)
{
	int len;
	uint16_t *buff;
	uint16_t response_buff[65000];
	MusicMeta *server_res;

	buff = htonmm(mm, hints); // FIXME: list with no filter causes malloc error after op
	len = (int) hints->pkt_size;
	sendall(fd, buff, &len);


	printf("waiting response\n");
	recvall(fd, response_buff, 650000, 0);
	server_res = ntohmm(response_buff, hints);

	free(buff);

	if (hints->pkt_type == MUSIC_RES) {
		printf("server responded op %d with status %d\n", hints->pkt_op, hints->pkt_status);
	}

	return server_res;
}

void talk_udp(MusicMeta *mm, MMHints *hints, int fd, struct addrinfo *p)
{	
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	int len, count;
	uint16_t *buff;
	uint16_t frag, total;
	uint16_t res[1024 * 1024];

	buff = htonmm(mm, hints);
	len = (int) hints->pkt_size;

	// FIXME: should be sendall
	if (sendto(fd, buff, len, 0, p->ai_addr, p->ai_addrlen) == -1) {
		perror("failed to send udp pkt");
		exit(1);
	}
	printf("waiting response\n");

	free(buff);

	count = 0;
	total = 0;
	int n;
	while (1) {
		if ((n = recvfrom(fd, res, (4 + FRAG_SIZE) * 2, 0, (struct sockaddr *) &their_addr, &sin_size)) == -1) {
			perror("recvfrom");
			exit(1);
		}
		frag = ntohs(res[3]) & 0b0000000011111111;
		total = (ntohs(res[3]) & 0b1111111100000000) >> 8;

		printf("client: received fragment %d/%d in %d bytes!\n", frag, total, n);
		count++;

		if (count >= total)
			break;
	}

	// recvfrom(fd, buff, 65000, 0,p->ai_addr, p->ai_addrlen);
	//pega o tamanho total e itera
	//vai colocando em ordem
	// server_res = ntohmm(response_buff, hints);


	// if (hints->pkt_type == MUSIC_RES) {
	// 	printf("server responded op %d with status %d\n", hints->pkt_op, hints->pkt_status);
	// }

	// return;
}

void handle_operation(char *op, int auth, int tcpfd, int udpfd, struct addrinfo *p)
{
	int i, error;
	char operations[][10] = {"add", "rem", "list", "download", "exit"};
	MusicMeta mm;
    MMHints hints;
	MusicMeta *response;

	for (i = 0; i < 5; i++) {
		if (strcmp(op, operations[i]) == 0) break;
	}

	memset(&mm, 0, sizeof(MusicMeta));
	memset(&hints, 0, sizeof(MMHints));
	
	switch (i)
	{
	case 0:
		error = handle_add(auth, &mm, &hints);
		if (error) return;
		response = talk_tcp(&mm, &hints, tcpfd);
		break;
	case 1:
		error = handle_remove(auth, &mm, &hints);
		if (error) return;
		response = talk_tcp(&mm, &hints, tcpfd);

		if(hints.pkt_status != MUSIC_OK){
			printf("\nNenhuma música com este id\n");
		}

		break;
	case 2:
		error = handle_list(1, &mm, &hints);
		if (error) return;
		response = talk_tcp(&mm, &hints, tcpfd);

		if (hints.pkt_type == MUSIC_RES) {

			if (hints.pkt_numres == 0)
				printf("Nenhuma música com essas carcterísticas encontrada!\n");
			else
				printf("Listando músicas com as características desejadas!\n");


			for (int i = 0; i < hints.pkt_numres; i++) {
				printf("\t%d, %d, %s, %s, %s, %s, %s\n",
					response[i].id,
					response[i].release_year,
					response[i].title,
					response[i].interpreter,
					response[i].language,
					response[i].category,
					response[i].chorus
				);
			}
		}
		
		break;
	case 3:
		error = handle_download(1, &mm, &hints);
		if (error) return;
		talk_udp(&mm, &hints, udpfd, p);
		break;
	case 4:
		error = handle_exit(1, &mm, &hints);
		if (error) return;
		talk_tcp(&mm, &hints, tcpfd);

		if (hints.pkt_status == MUSIC_OK) {
			exit(0);
		}

		break;
	default:
		printf("Operação não suportada. Tente novamente\n");
		return;
	}

	free(response);
}

int main(int argc, char *argv[])
{
	int sockfd, downloadfd;  
	struct addrinfo hints, *p;
	char op[10];
	char clear_line[128];
	int auth;

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
	auth = argc > 2 && strcmp(argv[2], "-adm") == 0 ? 1 : 0;
	greet_prompt(auth);

	while (1){
		printf("Digite a operação: ");
		scanf(" %s", op);
		fgets(clear_line, 128, stdin);

		handle_operation(op, auth, sockfd, downloadfd, p);
	}

	close(sockfd);

	return 0;
}
