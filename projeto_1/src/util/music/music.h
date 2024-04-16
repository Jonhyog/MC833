#ifndef MUSIC_H_INCLUDED__
#define MUSIC_H_INCLUDED__

#include "parser.h"

#define STD_FIELD_SIZE 128

typedef struct {
    int  id;
    int  release_year;
    unsigned char title[STD_FIELD_SIZE];
    unsigned char interpreter[STD_FIELD_SIZE];
    unsigned char language[STD_FIELD_SIZE];
    unsigned char category[STD_FIELD_SIZE];
    unsigned char chorus[2 * STD_FIELD_SIZE];
    unsigned char fpath[STD_FIELD_SIZE];
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
int rmv_music(MusicLib *ml, int id);
MusicMeta * get_meta(MusicLib *ml, MusicMeta *mm, int filter, int *res_size);

#endif