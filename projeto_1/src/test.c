#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "parser.h"
#include "music.h"
#include "codec.h"

void print_bin(uint16_t n)
{
    for (int i = 15; i >= 0; --i) {
        printf("%d", n >> i & 1);
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
	    fprintf(stderr,"test: fpath\n");
	    exit(1);
	}

    MusicLib *ml = newlib(128);
    CSV *csv = newcsv(128, 8);
    FILE *stream = fopen(argv[1], "r");

    parsecsv(stream, csv, ";\n", 0);
    loadmusics(ml, csv);

    for (int i = 0; i < csv->lines; i++) {
        printf("%d, %d, %s, %s, %s, %s, %s, %s\n",
            ml->musics[i].meta.id,
            ml->musics[i].meta.release_year,
            ml->musics[i].meta.title,
            ml->musics[i].meta.interpreter,
            ml->musics[i].meta.language,
            ml->musics[i].meta.category,
            ml->musics[i].meta.chorus,
            ml->musics[i].meta.fpath
        );

        MMHints hints;

        hints.pkt_filter = 0;
        hints.pkt_op = MUSIC_GET;
        hints.pkt_numres = 1;
        hints.pkt_status = 0;

        MusicMeta mm = ml->musics[i].meta;
        uint16_t *data = htonmm(&mm, &hints);

        FILE *write_ptr;

        write_ptr = fopen("dump.bin", "wb");
        fwrite(data, sizeof(uint16_t) * hints.pkt_size, 1, write_ptr);

        MusicMeta *res = ntohmm(data, &hints);

        printf("RES\n%d, %d, %s, %s, %s, %s, %s, %s\n",
            res[0].id,
            res[0].release_year,
            res[0].title,
            res[0].interpreter,
            res[0].language,
            res[0].category,
            res[0].chorus,
            res[0].fpath
        );

        printf("HINTS:\n\tSIZE:%d\n\tTYPE:%d\n\tOP:%d\n\tSTATUS:%d\n\tNUM_RES:%d\n\tFILTER:%d\n",
            hints.pkt_size,
            hints.pkt_type,
            hints.pkt_op,
            hints.pkt_status,
            hints.pkt_numres,
            hints.pkt_filter
        );
    }

}