#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"

CSV *newcsv(int max_lines, int max_collumns)
{
    CSV *csv = (CSV *) malloc(sizeof(CSV));

    csv->max_lines = max_lines;
    csv->max_collumns = max_collumns;

    csv->lines = csv->collumns = 0;

    csv->data = (char ***) malloc(sizeof(char **) * csv->max_lines);
    for (int i = 0; i < csv->max_lines; i++) {
        csv->data[i] = (char **) malloc(sizeof(char *) * csv->max_collumns);

        for (int j = 0; j < csv->max_collumns; j++) {
            csv->data[i][j] = (char *) malloc(sizeof(char) * 128);
        }
    }

    return csv;
}

void parsecsv(FILE *stream, CSV *dest, char *sep, int skip_header)
{
    char buff[1024];

    if (skip_header) fgets(buff, 1024, stream);

    // Finds number of colls
    if (fgets(buff, 1024, stream)) {
        for (char *tok = strtok(buff, sep); tok && *tok; tok = strtok(NULL, sep)) {
            strcpy(
                dest->data[dest->lines][dest->collumns],
                tok
            );
            dest->collumns++;
        }
        dest->lines++;
    }

    // Parses remaining data
    while (fgets(buff, 1024, stream)) {
        int collumns = 0;
        for (char *tok = strtok(buff, sep); tok && *tok; tok = strtok(NULL, sep)) {
            strcpy(
                dest->data[dest->lines][collumns],
                tok
            );
            collumns++;
        }
        if (collumns != 0) dest->lines++;
    }
}

void savecsv(FILE *stream, CSV *csv, char *sep)
{
    char line[2048];
    int line_ptr;

    for (int i = 0; i < csv->lines; i++) {
        memset(line, 0, 2048);
        line_ptr = 0;
        for (int j = 0; j < csv->collumns; j++) {
            for (int k = 0; k < strlen(csv->data[i][j]); k++) {
                line[line_ptr] = csv->data[i][j][k];
                line_ptr++;
            }
            line[line_ptr] = sep[0];
            line_ptr++;
        }
        line[line_ptr] = '\n';
        line[line_ptr + 1] = '\0';
        fprintf(stream, line, "%s");
    }
}