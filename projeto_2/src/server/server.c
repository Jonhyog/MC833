#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#include "parser.h"
#include "music.h"
#include "codec.h"

#define PORT "3490"

#define BACKLOG 10

#define BUFFER_SIZE 2048

#define MAX(A, B) (A > B) ? A : B

MusicLib *db;
FILE *stream;
CSV *csv;
char fpath[512];

void sigchld_handler(int s)
{
	(void)s; // quiet unused variable warning

	// waitpid() might overwrite errno, so we save and restore it:
	int saved_errno = errno;

	while(waitpid(-1, NULL, WNOHANG) > 0);

	errno = saved_errno;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

int create_socket(char *port, struct addrinfo hints)
{
	int sockfd;
	int rv, yes = 1;
	struct addrinfo *servinfo, *p;

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	freeaddrinfo(servinfo);

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		exit(1);
	}

	return sockfd;
}

void service_loop(int fd, MusicLib *db)
{
    MusicMeta *mm;
    MMHints hints;
    uint16_t buff[BUFFER_SIZE];
	uint16_t *resbuff;
	MusicMeta *res;
	MusicData *temp;
	int len, numres;
	int end = 0;
    
    while (1) {
        // gets all data
        recvall(fd, buff, BUFFER_SIZE, 0);

        // unpacks data
        memset(&hints, 0, sizeof(MMHints));
        mm = ntohmm(buff, &hints);

		// handles operations
		switch (hints.pkt_op)
		{
		case MUSIC_ADD:
			printf("server: adding music \n");
			printf("\t%d, %d, %s, %s, %s, %s, %s\n",
				mm->id,
				mm->release_year,
				mm->title,
				mm->interpreter,
				mm->language,
				mm->category,
				mm->chorus
			);
			temp = (MusicData *) malloc(sizeof(MusicData));
			temp->data_size = 0;
			meta_copy(&temp->meta, mm);
			add_music(db, temp);
			free(temp);

			// sets response hints
			hints.pkt_type = MUSIC_RES;
			hints.pkt_status = MUSIC_OK;
			hints.pkt_numres = 0;
			res = NULL;

			break;
		case MUSIC_DEL:

			// sets response hints
			hints.pkt_type = MUSIC_RES;
			if(rmv_music(db, mm->id) == 1){
				hints.pkt_status = MUSIC_ERR;
			}
			else{
				hints.pkt_status = MUSIC_OK;
			}	
			hints.pkt_numres = 0;
			res = NULL;

			break;
		case MUSIC_LIST:
			numres = 0;
            res = get_meta(db, mm, (int) hints.pkt_filter, &numres);

			printf("server: found %d musics with matching fields\n", numres);
			printf("server: listing musics with matching meta fields\n");
			for (int i = 0; i < numres; i++) {
				printf("%d, %d, %s, %s, %s, %s, %s, %s\n",
					res[i].id,
					res[i].release_year,
					res[i].title,
					res[i].interpreter,
					res[i].language,
					res[i].category,
					res[i].chorus,
					res[i].fpath
				);
			}
			printf("server: stopped listing\n");

			// sets response hints
			hints.pkt_type = MUSIC_RES;
			hints.pkt_status = MUSIC_OK;
			hints.pkt_numres = numres;

			break;
		case MUSIC_CLOSE:
			// sets response hints
			hints.pkt_type = MUSIC_END;
			hints.pkt_status = MUSIC_OK;
			hints.pkt_numres = 0;
			res = NULL;

			end = 1;

			break;
		default:
			break;
		}

		// responds to client
		resbuff = htonmm(res, &hints);
		len = (int) hints.pkt_size;
		printf("server: responding op %d with %d bytes\n", hints.pkt_op, hints.pkt_size);
		sendall(fd, resbuff, &len);

		if (res != NULL) free(res);

		if (db->size == 0) {
			printf("server: no data...\n");
		}

		if (end) {
			struct sockaddr_in their_addr; // connector's address information
			socklen_t sin_size;
			char next_id[10];

			// info
			getpeername(fd, (struct sockaddr *) &their_addr, &sin_size);
			printf("server: end conection with %s\n", inet_ntoa(their_addr.sin_addr));

			// saves date before leaving
			printf("server: saving data in %s\n", fpath);
			savemusics(db, csv);
			stream = fopen(fpath, "w");
			savecsv(stream, csv, ";");

			// saves db metadata
			stream = fopen("music_lib.meta", "w");
			sprintf(next_id, "%d\n", db->next_id);
			fprintf(stream, next_id, "%s");

			printf("server: save complete\n");

			close(fd);
			exit(0);
		}

        for (int i = 0; i < db->size; i++) {
            printf("%d, %d, %s, %s, %s, %s, %s, %s\n",
                db->musics[i].meta.id,
                db->musics[i].meta.release_year,
                db->musics[i].meta.title,
                db->musics[i].meta.interpreter,
                db->musics[i].meta.language,
                db->musics[i].meta.category,
                db->musics[i].meta.chorus,
                db->musics[i].meta.fpath
            );
        }
    }
}

// int file_size(FILE *file){
// 	fseek(file, 0L, SEEK_END);
// 	int size = ftell(file);
// 	rewind(file);
// 	return size;
// }

// char *header(int c){
// 	int header_size = 10; //????????
// 	char *str;
// 	for(int i=0;i<strlen(str) - header_size;i++){
// 		str[i] = ' ';
// 	}
// 	strcat(str, c + '0');
// 	return str;
// }
// 
// int send_song(char *filename, int sockfd, const struct sockaddr *to, socklen_t *tolen){
// 	FILE *file;
// 	int max_size = 10000; //???????????????
// 	file = fopen(filename,"rb");
// 	int count = file_size(file)/max_size;
// 	char *header_count = header(count);
// 	int i = 0;
// 	for (i=0;i<count;i++){
// 		char *buf;
// 		char *file_chunk;
// 		strcpy(buf, header_count);
// 		strcat(buf, header(i));
// 		fread(&file_chunk, 1, max_size,file);
// 		strcat(buf, file_chunk);
// 		sendto(sockfd, buf, max_size + 20, 0, to, tolen); 
// 	}
// 	fclose(file);
	
// }

void send_song(char *fname, int sockfd, struct sockaddr *to, socklen_t *tolen)
{
	FILE *file;
	uint16_t **pkts;
	MMHints hints;
	int frags;

	file = fopen(fname, "rb");

	int size;

	fseek(file, 0L, SEEK_END);
	size = ftell(file);
	rewind(file);

	printf("server: got fd %p for file\n", file);
	printf("server: file has %d bytes\n", size);

	// sets response hints and creates pkts
	hints.pkt_op = MUSIC_GET;
	hints.pkt_type = MUSIC_RES;
	hints.pkt_status = MUSIC_OK;
	hints.pkt_filter = 0;
	hints.pkt_numres = 0;
	pkts = htonmd(file, &hints, &frags);

	printf("server: sending %d fragments to client\n", frags);
	// sends all fragments
	for (int i = 0; i < frags; i++) {
		printf("\rserver: sending fragment %d", i);
		fflush(stdout);
		sendto(sockfd, pkts[i], (4 + FRAG_SIZE) * 2, 0, to, *tolen);
	}
	printf("\n");
}

int main(int argc, char *argv[])
{
	int sockfd, new_fd, downloadfd, maxfdp1;  // listen on sock_fd, new connection on new_fd
	int nready;
	fd_set rset;
	struct addrinfo hints;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	char s[INET6_ADDRSTRLEN];
	char next_id[10];

    if (argc != 2) {
        fprintf(stderr, "usage: server musics.csv\n");
        exit(1);
    }
    
    // starts up sockets
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	sockfd = create_socket(PORT, hints);

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE;
	downloadfd = create_socket(PORT, hints);
	
	if (listen(sockfd, BACKLOG) == -1) {
		perror("listen");
		exit(1);
	}

	sa.sa_handler = sigchld_handler; // reap all dead processes
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGCHLD, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

    // main accept() loop
	printf("server: waiting for connections...\n");
	FD_ZERO(&rset);
	maxfdp1 = MAX(sockfd, downloadfd) + 1;
	while (1) {
		FD_SET(sockfd, &rset);
		FD_SET(downloadfd, &rset);

		if ((nready = select(maxfdp1, &rset, NULL, NULL, NULL)) < 0) {
			if (errno == EINTR)
				continue;
			else {
				perror("select_error");
				exit(1);
			}
		}

		// TCP Service Loop
		if (FD_ISSET(sockfd, &rset)) {
			sin_size = sizeof their_addr;
			new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
			if (new_fd == -1) {
				perror("accept");
				continue;
			}

			inet_ntop(their_addr.ss_family,
				get_in_addr((struct sockaddr *)&their_addr),
				s, sizeof s);
			printf("server: got connection from %s\n", s);

			if (!fork()) {
				close(sockfd);

				db = newlib(128);
				csv = newcsv(128, 8);

				// loads db metadata
				stream = fopen("music_lib.meta", "r");
				fgets(next_id, 10, stream);
				db->next_id = atoi(next_id);
				
				// loads musics data to memory
				stream = fopen(argv[1], "r");
				strcpy(fpath, argv[1]);
				parsecsv(stream, csv, ";\n", 0);
				loadmusics(db, csv);

				service_loop(new_fd, db);
				close(new_fd);
				exit(0);
			}

			close(new_fd);
		}

		// UDP Service Loop
		if (FD_ISSET(downloadfd, &rset)) {
			// char music_name[128];
			uint16_t buff[1000];
			MusicMeta *mm;
    		MMHints hints;
			sin_size = sizeof their_addr;
			int n = recvfrom(downloadfd, buff, 1000, 0, (struct sockaddr *) &their_addr, &sin_size);

			printf("server: received %d bytes using UDP!\n", n);

			// unpacks data
			memset(&hints, 0, sizeof(MMHints));
			mm = ntohmm(buff, &hints);

			if (hints.pkt_op == MUSIC_GET) {
				printf("server: sending file for music %d\n", mm->id);
				// get_music_name(music_name, db, mm->id);
				send_song("songs/test.mp3", downloadfd, (struct sockaddr *) &their_addr, &sin_size);

				// char *filename = download_music(db, mm->id);
				// if(strcmp(filename, "")){
				// 	hints.pkt_status = MUSIC_ERR;
				// }
				// else{
				// 	hints.pkt_status = MUSIC_OK;
				// }	
				// hints.pkt_numres = 0;
				// send_song(filename, downloadfd, &their_addr, sin_size);
				
			} else {
				perror("server: operation is not supported via UDP\n");

			}
		}
	}

	return 0;
}
