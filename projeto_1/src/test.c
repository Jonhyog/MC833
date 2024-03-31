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

        int size = 76;
        uint16_t *data = htonmm(ml->musics[i].meta, 1, MUSIC_GET, 0);

        FILE *write_ptr;

        write_ptr = fopen("dump.bin", "wb");
        fwrite(data, sizeof(uint16_t) * size, 1, write_ptr);

        printf("h1: "); print_bin(data[0]);
        printf("h2: "); print_bin(data[1]);
        
        for (int i = 0; i < 8 * 1; i++) {
            printf("offset(%d): %d\t- 0b", i, data[2 + i]); print_bin(data[2 + i]);
        }

        printf("id: \t%d\n", data[10]);
        printf("release_year: \t%d\n", data[11]);

        printf("title: \t");
        for (int i = 0; data[12 + i] != '\0'; i++)
            printf("%c%c", (data[12 + i] << 8) >> 8, data[12 + i] >> 8);
        printf("\n");
        break;
    }

}