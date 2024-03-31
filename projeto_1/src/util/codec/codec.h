#ifndef CODEC_H_INCLUDED__
#define CODEC_H_INCLUDED__

#include <stdint.h>

#include "music.h"

#define HEADER_SIZE 12

#define MUSIC_ADD  0b00
#define MUSIC_DEL  0b01
#define MUSIC_LIST 0b10
#define MUSIC_GET  0b11

uint16_t* htonmm(MusicMeta mm, uint16_t size, uint16_t op, uint16_t filter);
// unsigned char* htonmd(MusicData md);

MusicMeta* ntohmm(unsigned char* pkt);
// MusicMeta* ntohmd(unsigned char* pkt);


#endif