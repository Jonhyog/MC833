#ifndef CODEC_H_INCLUDED__
#define CODEC_H_INCLUDED__

#include <stdint.h>

#include "music.h"

#define HEADER_SIZE 12

#define MUSIC_ADD  0b00
#define MUSIC_DEL  0b01
#define MUSIC_LIST 0b10
#define MUSIC_GET  0b11

// pkt_size     : 16 bits

// pkt_type     : 04 bits
// pkt_op       : 04 bits
// pkt_status   : 08 bits

// pkt_filter   : 08 bits
// pkt_numres   : 08 bits

// repete pkt_numres vezes
// pkt_secoff   : 16 bits (offset relativo ao inicio do pacote)
// pkt_idoff    : 16 bits (offset relativo ao inicio da seção )
// pkt_rloff    : 16 bits (offset relativo ao inicio da seção )
// pkt_ttoff    : 16 bits (offset relativo ao inicio da seção )

// pkt_itoff    : 16 bits (offset relativo ao inicio da seção )
// pkt_lgoff    : 16 bits (offset relativo ao inicio da seção )

// pkt_ctoff    : 16 bits (offset relativo ao inicio da seção )
// pkt_choff    : 16 bits (offset relativo ao inicio da seção )

typedef struct {
    uint16_t pkt_size;
    uint16_t pkt_type;
    uint16_t pkt_op;
    uint16_t pkt_status;
    uint16_t pkt_numres;
    uint16_t pkt_filter;
} MMHints;

uint16_t* htonmm(MusicMeta *mm, MMHints *hints);
// unsigned char* htonmd(MusicData md);

MusicMeta* ntohmm(uint16_t* pkt, MMHints *hints);
// MusicMeta* ntohmd(unsigned char* pkt);


#endif