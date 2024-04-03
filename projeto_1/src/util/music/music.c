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
    ml->next_id = 1;
    ml->musics = (MusicData *) malloc(sizeof(MusicData) * max_size);

    return ml;
}

void setmeta(MusicMeta *mm, char **params)
{
    mm->id = atoi(params[0]);
    mm->release_year = atoi(params[1]);
    
    strcpy((char *) mm->title, params[2]);
    strcpy((char *) mm->interpreter, params[3]);
    strcpy((char *) mm->language, params[4]);
    strcpy((char *) mm->category, params[5]);
    strcpy((char *) mm->chorus, params[6]);
    strcpy((char *) mm->fpath, params[7]);
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

void meta_copy(MusicMeta *dest, MusicMeta *src)
{   
    // integer fields
    dest->id = src->id;
    dest->release_year = src->release_year;
    
    // string fields
    strcpy((char *) dest->title,       (char *) src->title);
    strcpy((char *) dest->interpreter, (char *) src->interpreter);
    strcpy((char *) dest->language,    (char *) src->language);
    strcpy((char *) dest->category,    (char *) src->category);
    strcpy((char *) dest->chorus,      (char *) src->chorus);
    strcpy((char *) dest->fpath,       (char *) src->fpath);
}

void msc_copy(MusicData *dest, MusicData *src)
{
    for (int i = 0; src->data_size; i++) dest->data[i] = src->data[i];
    dest->data_size = src->data_size;

    meta_copy(&dest->meta, &src->meta);
}

void add_music(MusicLib *ml, MusicData *md)
{
    msc_copy(&ml->musics[ml->size], md);
    ml->musics[ml->size].meta.id = ml->next_id;
    ml->next_id++;
    ml->size++;
}

void rmv_music(MusicLib *ml, int id)
{
    int pos = -1;

	for (int i = 0; i < ml->size; i++) {
		if (ml->musics[i].meta.id == id) {
			pos = i;
			break;
		}
	}

    // FIX-ME: should set a error
	if (pos == - 1) return;

	for (int i = pos; i < ml->size - 1; i++)
        msc_copy(&ml->musics[i], &ml->musics[i + 1]);
	ml->size--;
}

int compare_field(MusicMeta *a, MusicMeta *b, int filter)
{
    int res = 1;

    switch (filter)
    {
    case (0b1 << 0):
        res = (a->id != b->id);
        break;
    case (0b1 << 1):
        res = (a->release_year != b->release_year);
        break;
    case (0b1 << 2):
        res = strcmp((char *) a->title, (char *) b->title);
        break;
    case (0b1 << 3):
        res = strcmp((char *) a->interpreter, (char *) b->interpreter);
        break;
    case (0b1 << 4):
        res = strcmp((char *) a->language, (char *) b->language);
        break;
    case (0b1 << 5):
        res = strcmp((char *) a->category, (char *) b->category);
        break;
    case (0b1 << 6):
        res = strcmp((char *) a->chorus, (char *) b->chorus);
        break;
    default:
        // FIX-ME: should set errno because filter is invalid
        break;
    }

    return res;
}

MusicMeta * get_meta(MusicLib *ml, MusicMeta *mm, int filter, int *res_size)
{
	MusicMeta *res = (MusicMeta *) calloc(ml->size, sizeof(MusicMeta));

	*res_size = 0;
	for (int i = 0; i < ml->size; i++) {

        int found = 1;
        for (int j = 0; j < 7; j++) {
            int filter_j = filter & (0b1 << j);
            // not filtering by j-eth field

            if (filter_j == 0) continue;

            // compares j-eth field
            if (compare_field(&ml->musics[i].meta, mm, filter_j) != 0) {
                found = 0;
                break;
            }
        }

        if (found) {
            meta_copy(&res[*res_size], &ml->musics[i].meta);
            printf("id check: %d x %d\n", res[*res_size].id, ml->musics[i].meta.id);
		    *res_size += 1;
        }
	}

    printf("server: found %d musics with matching fields\n", *res_size);
	return res;
}
