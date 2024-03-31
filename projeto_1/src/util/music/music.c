#include <stdlib.h>
#include <string.h>

#include "music.h"

MusicMeta* newmeta(char **params)
{
    MusicMeta *mm = (MusicMeta *) malloc(sizeof(MusicMeta));
    memset(mm, 0, sizeof(MusicMeta));

    if (params != NULL) setmeta(mm, params);

    return mm;
}

MusicLib* newlib(int max_size)
{
    MusicLib *ml = (MusicLib *) malloc(sizeof(MusicLib));

    ml->max_size = max_size;
    ml->size = 0;
    ml->musics = (MusicData *) malloc(sizeof(MusicData) * max_size);

    return ml;
}

void setmeta(MusicMeta *mm, char **params)
{
    mm->id = atoi(params[0]);
    mm->release_year = atoi(params[1]);
    
    strcpy(mm->title, params[2]);
    strcpy(mm->interpreter, params[3]);
    strcpy(mm->language, params[4]);
    strcpy(mm->category, params[5]);
    strcpy(mm->chorus, params[6]);
    strcpy(mm->fpath, params[7]);
}

void loadmusics(MusicLib *ml, CSV *db)
{
    // TODO: Throw error if max < db->lines

    // sets up music lib
    ml->size = db->lines;

    for (int i = 0; i < ml->size; i++) {
        setmeta(&ml->musics[i].meta, db->data[i]);
    }
}
