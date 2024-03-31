#ifndef MUSIC_H_INCLUDED__
#define MUSIC_H_INCLUDED__

#include "parser.h"

#define MAX_FIELD_SIZE 128

typedef struct {
    int  id;
    int  release_year;
    unsigned char title[MAX_FIELD_SIZE];
    unsigned char interpreter[MAX_FIELD_SIZE];
    unsigned char language[MAX_FIELD_SIZE];
    unsigned char category[MAX_FIELD_SIZE];
    unsigned char chorus[MAX_FIELD_SIZE];
    unsigned char fpath[MAX_FIELD_SIZE];
} MusicMeta;

typedef struct {
    MusicMeta meta;
    unsigned char *data;
} MusicData;

typedef struct {
    int max_size;
    int size;
    MusicData *musics;
} MusicLib;

MusicMeta* newmeta(char **params);
MusicLib* newlib(int max_size);
void setmeta(MusicMeta *mm, char **params);
void loadmusics(MusicLib *md, CSV *db);

#endif