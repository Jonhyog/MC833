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

// void int_handler(int sig)
// {
// 	char  c;

// 	signal(sig, SIG_IGN);
// 	printf("server: Deseja sair? (y/n)\n");
// 	c = getchar();
// 	if (c == 'y' || c == 'Y') {
// 		savemusics(db, csv);
// 		stream = fopen(fpath, "w");
// 		savecsv(stream, csv, ";");
// 		exit(0);
// 	} else {
// 		signal(SIGINT, int_handler);
// 	}
// }

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void handle_add(MusicLib *db, MusicMeta *mm)
{
	// FIX-ME: db should decide choose id value

    MusicMeta *db_meta = &db->musics[db->size].meta;

    db_meta->id = mm->id;
    db_meta->release_year = mm->release_year;

    strcpy((char *) db_meta->title, (char *) mm->title);
    strcpy((char *) db_meta->interpreter, (char *) mm->interpreter);
    strcpy((char *) db_meta->language, (char *) mm->language);
    strcpy((char *) db_meta->category, (char *) mm->category);
    strcpy((char *) db_meta->chorus, (char *) mm->chorus);

    db->size++;
}

void handle_delete(MusicLib *db, MusicMeta *mm)
{
	int pos = -1;

	for (int i = 0; i < db->size; i++) {
		if (db->musics[i].meta.id == mm->id) {
			pos = i;
			break;
		}
	}

	if (pos == - 1) {
		printf("server: no music with id == %d \n/", mm->id);
		return;
	}

	for (int i = pos; i < db->size - 1; i++) {
		db->musics[i] = db->musics[i + 1];
	}
	db->size--;
}

MusicMeta * handle_list(MusicLib *db, MusicMeta *mm, MMHints hints, uint16_t *res_size)
{
	uint16_t filter = hints.pkt_filter;
	MusicMeta *res = (MusicMeta *) calloc(db->size, sizeof(MusicMeta));

	*res_size = 0;
	for (int i = 0; i < db->size; i++) {
		
		if (((filter >> 0) & 1) && db->musics[i].meta.id != mm->id) continue;
		if (((filter >> 1) & 1) && db->musics[i].meta.release_year != mm->release_year) continue;
		if (((filter >> 2) & 1) && !strcmp((char *) db->musics[i].meta.title, (char *) mm->title)) continue;
		if (((filter >> 3) & 1) && !strcmp((char *) db->musics[i].meta.interpreter, (char *) mm->interpreter)) continue;
		if (((filter >> 4) & 1) && !strcmp((char *) db->musics[i].meta.language, (char *) mm->language)) continue;
		if (((filter >> 5) & 1) && !strcmp((char *) db->musics[i].meta.category, (char *) mm->category)) continue;
		if (((filter >> 6) & 1) && !strcmp((char *) db->musics[i].meta.chorus, (char *) mm->chorus)) continue;
		
		res[*res_size].id = db->musics[i].meta.id;
    	res[*res_size].release_year = db->musics[i].meta.release_year;

		strcpy((char *) res[*res_size].title, (char *) db->musics[i].meta.title);
		strcpy((char *) res[*res_size].interpreter, (char *) db->musics[i].meta.interpreter);
		strcpy((char *) res[*res_size].language, (char *) db->musics[i].meta.language);
		strcpy((char *) res[*res_size].category, (char *) db->musics[i].meta.category);
		strcpy((char *) res[*res_size].chorus, (char *) db->musics[i].meta.chorus);

		*res_size += 1;
	}

	return res;
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
			temp = (MusicData *) malloc(sizeof(MusicData));
			meta_copy(&temp->meta, mm);
			add_music(db, temp);
			free(temp);

			// sets response hints
			hints.pkt_type = MUSIC_RES;
			hints.pkt_status = MUSIC_OK;
			hints.pkt_numres = 0;
			res = NULL;

            // FILE *write_ptr;
            // write_ptr = fopen("server_dump.bin", "wb");
            // fwrite(buff, hints.pkt_size, 1, write_ptr);

			break;
		case MUSIC_DEL:
			rmv_music(db, mm->id);

			// sets response hints
			hints.pkt_type = MUSIC_RES;
			hints.pkt_status = MUSIC_OK;
			hints.pkt_numres = 0;
			res = NULL;

			break;
		case MUSIC_LIST:
			numres = 0;
            res = get_meta(db, mm, (int) hints.pkt_filter, &numres);

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

int main(int argc, char *argv[])
{
	int sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	struct sigaction sa;
	int yes=1;
	char s[INET6_ADDRSTRLEN];
	char next_id[10];
	int rv;

    // FIX-ME: parsecsv and loadmusics should return pointer to csv and db
    // MusicLib *db = newlib(128);
    // FILE *stream;
    // CSV *csv = newcsv(128, 8);
    // db = newlib(128);
    // csv = newcsv(128, 8);

    if (argc != 2) {
        fprintf(stderr, "usage: server musics.csv\n");
        exit(1);
    }
    
	// // loads db metadata
	// stream = fopen("music_lib.meta", "r");
	// fgets(next_id, 10, stream);
	// db->next_id = atoi(next_id);
	
    // // loads musics data to memory
    // stream = fopen(argv[1], "r");
	// strcpy(fpath, argv[1]);
    // parsecsv(stream, csv, ";\n", 0);
    // loadmusics(db, csv);

    // starts up sockets
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

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

	// catches ctrl+c to safely exit
	// signal(SIGINT, int_handler);

    // main accept() loop
	printf("server: waiting for connections...\n");
	while (1) {
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

	return 0;
}
