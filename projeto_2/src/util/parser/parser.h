#ifndef PARSER_H_INCLUDED__
#define PARSER_H_INCLUDED__

#include <stdio.h>

typedef struct {
    int  max_lines;
    int  max_collumns;
    int  lines;
    int  collumns;
    char ***data;
} CSV;

CSV *newcsv(int max_lines, int max_collumns);
void parsecsv(FILE *stream, CSV *dest, char *sep, int skip_header);
void savecsv(FILE *stream, CSV *csv, char *sep);

#endif
