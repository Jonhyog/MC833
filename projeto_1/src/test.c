#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "music.h"

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
    }

}