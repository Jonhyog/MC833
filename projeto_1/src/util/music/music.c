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

void clearmeta(MusicMeta *mm)
{
    mm->id = 0;
    mm->release_year = 0;
    
    mm->title[0] = '\0';
    mm->interpreter[0] = '\0';
    mm->language[0] = '\0';
    mm->category[0] = '\0';
    mm->chorus[0] = '\0';
    mm->fpath[0] = '\0';
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

void savemusics(MusicLib *ml, CSV *db)
{
    // updates db size
    db->lines = ml->size;
    db->collumns = 8;

    // copies data to db
    for (int i = 0; i < db->lines; i++) {
        sprintf(db->data[i][0], "%d", ml->musics[i].meta.id);
        sprintf(db->data[i][1], "%d", ml->musics[i].meta.release_year);
        strcpy(db->data[i][2], (char *) ml->musics[i].meta.title);
        strcpy(db->data[i][3], (char *) ml->musics[i].meta.interpreter);
        strcpy(db->data[i][4], (char *) ml->musics[i].meta.language);
        strcpy(db->data[i][5], (char *) ml->musics[i].meta.category);
        strcpy(db->data[i][6], (char *) ml->musics[i].meta.chorus);
        // strcpy(db->data[i][7], ""); // placeholder for fpath
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

int rmv_music(MusicLib *ml, int id)
{
    int pos = -1;

	for (int i = 0; i < ml->size; i++) {
		if (ml->musics[i].meta.id == id) {
			pos = i;
			break;
		}
	}

    // FIX-ME: should set a error
	if (pos == - 1) return 1;

	for (int i = pos; i < ml->size - 1; i++)
        msc_copy(&ml->musics[i], &ml->musics[i + 1]);
	ml->size--;
    return 0;
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
    MusicMeta *res = (MusicMeta *) malloc(sizeof(MusicMeta) * ml->size);

    for (int i = 0; i < ml->size; i++) clearmeta(&res[i]);

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
		    *res_size += 1;
        }
	}

	return res;
}
