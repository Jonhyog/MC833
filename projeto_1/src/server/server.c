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

void recvall(int fd, uint16_t *buff, int flags)
{
    int size = 0;
    uint16_t pkt_size = -1;

    while (1) {
        if ((size += recv(fd, buff, BUFFER_SIZE - 1, 0)) == -1) {
            perror("recv");
            exit(1);
	    }

        if (size > 1 && pkt_size != -1) {
            pkt_size = ntohs(buff[0]);
            // printf("Expecting %d bytes\n", pkt_size);
        }

        if (size == pkt_size) {
            break;
        }

        // FIX-ME: should timeout if receives something
        // but takes too long (e.g. > 10s) to recv all pkt
    }

    printf("server: received %d bytes pkt\n", size);
}

void handle_add(MusicLib *db, MusicMeta *mm)
{
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

void service_loop(int fd, MusicLib *db)
{
    MusicMeta *mm;
    MMHints hints;
    uint16_t buff[BUFFER_SIZE];
    
    while (1) {
        // gets all data
        recvall(fd, buff, 0);

        // unpacks data
        memset(&hints, 0, sizeof(MMHints));
        mm = ntohmm(buff, &hints);

        if (hints.pkt_op == MUSIC_ADD) {
            handle_add(db, mm);

            FILE *write_ptr;
            write_ptr = fopen("server_dump.bin", "wb");
            fwrite(buff, hints.pkt_size, 1, write_ptr);
            // printf("DEBUG: %s\n", mm->interpreter);
            // exit(1);
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
	int rv;

    // FIX-ME: parsecsv and loadmusics should return pointer to csv and db
    MusicLib *db = newlib(128);
    FILE *stream;
    CSV *csv = newcsv(128, 8);;

    if (argc != 2) {
        fprintf(stderr, "usage: server musics.csv\n");
        exit(1);
    }
    
    // loads musics data to memory
    stream = fopen(argv[1], "r");
    parsecsv(stream, csv, ";\n", 0);
    loadmusics(db, csv);

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

	printf("server: waiting for connections...\n");

    // main accept() loop
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
            service_loop(new_fd, db);
			close(new_fd);
			exit(0);
		}

		close(new_fd);
	}

	return 0;
}
