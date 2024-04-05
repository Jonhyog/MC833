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
    int data_size;
    unsigned char *data;
} MusicData;

typedef struct {
    int max_size;
    int size;
    int next_id;
    MusicData *musics;
} MusicLib;

MusicMeta* newmeta(char **params);
MusicLib* newlib(int max_size);
void setmeta(MusicMeta *mm, char **params);
void loadmusics(MusicLib *md, CSV *db);
void savemusics(MusicLib *ml, CSV *db);

void meta_copy(MusicMeta *dest, MusicMeta *src);
void msc_copy(MusicData *a, MusicData *b);

void add_music(MusicLib *ml, MusicData *md);
void rmv_music(MusicLib *ml, int id);
MusicMeta * get_meta(MusicLib *ml, MusicMeta *mm, int filter, int *res_size);

#endif