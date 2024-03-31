#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <netinet/in.h>

#include "music.h"
#include "codec.h"

#define BITPACK(A, B) ((A - 1) | (B - 1)) + 1

void write2int16(uint16_t start, unsigned char *data, uint16_t *dest)
{
    // FIX-ME: might cause buffer overflow if strlen is odd
    // works because arrays always have size 128 so even
    // if strlen is odd next position is valid and null

    for (int j = 0; j < strlen((char *) data); j += 2) {
        dest[start] = data[j] | data[j + 1] << 8;
        start += 1;
    }
}

void read2char(uint16_t start, uint16_t *pkt, unsigned char *dest)
{
    // FIX-ME: might cause buffer overflow if value on pkt
    // is bigger than dest allocated size
    // works because dest always have size 128 and
    // protocol limits to strings of 128 bytes

    int j;
    for (j = 0; pkt[start] != '\0'; j += 2) {
        dest[j + 1] = (pkt[start] & 0b1111111100000000) >> 8;
        dest[j]     =  pkt[start] & 0b0000000011111111;
        start++;
    }
    dest[j] = '\0';
}

uint16_t* htonmm(MusicMeta *mm, MMHints *hints)
{   
    uint16_t pkt_size = 1;
    uint16_t offset_idx = 0;

    // header section
    uint16_t h1 = hints->pkt_type | (hints->pkt_op << 4) | (hints->pkt_status << 8);                     /* type == 0 && status == 0 */
    uint16_t h2 = hints->pkt_filter | (hints->pkt_numres << 8);
    pkt_size += 2;

    // offset table section
    uint16_t *offsets = (uint16_t *) calloc(8 * hints->pkt_numres, sizeof(uint16_t));
    pkt_size += 8 * hints->pkt_numres;

    // data sections
    uint16_t **data = (uint16_t **) calloc(hints->pkt_numres, sizeof(uint16_t *));
    uint16_t *section_size = (uint16_t *) calloc(hints->pkt_numres, sizeof(uint16_t));

    for (int i = 0; i < hints->pkt_numres; i++) {
        // secoff, idoff, rloff, ttoff, itoff, lgoff, ctoff, choff
        uint16_t fields_offsets[] = {0, 0, 0, 0, 0, 0, 0, 0};
        unsigned char *fields_values[] = {
            NULL,
            NULL,
            NULL,
            mm[i].title,
            mm[i].interpreter,
            mm[i].language,
            mm[i].category,
            mm[i].chorus
        };

        // sets section offset
        fields_offsets[0] = pkt_size;

        // id && release_year offsets
        fields_offsets[1] = 0;
        fields_offsets[2] = 1;
        section_size[i]  += 2;

        // string fields offsets
        for (int j = 3; j < 8; j++) {
            fields_offsets[j] = section_size[i];
            section_size[i]  += (BITPACK(strlen((char *) fields_values[j]) + 1, 16)) / 2;
        }

        // writes section offsets
        for (int j = 0; j < 8; j++) offsets[offset_idx + j] = fields_offsets[j];
        offset_idx += 8;

        // updates pkt size
        pkt_size += section_size[i];

        // allocates space for data
        data[i] = calloc(section_size[i], sizeof(uint16_t));

        // writes interger fields
        data[i][fields_offsets[1]] = mm[0].id;
        data[i][fields_offsets[2]] = mm[0].release_year;

        // writes string fields
        for (int j = 3; j < 8; j++) write2int16(fields_offsets[j], fields_values[j], data[i]);
    }
    
    uint16_t *pkt = calloc(pkt_size, sizeof(uint16_t));
    uint16_t pkt_idx = 3;

    // copies header
    pkt[0] = htons(pkt_size);
    pkt[1] = htons(h1);
    pkt[2] = htons(h2);

    // copies offset table
    for (int i = 0; i < 8 * hints->pkt_numres; i++) pkt[pkt_idx + i] = htons(offsets[i]);
    pkt_idx += 8 * hints->pkt_numres;

    // copies data to pkt
    for (int i = 0; i < hints->pkt_numres; i++) {
        for (int j = 0; j < section_size[i]; j++) {
            pkt[pkt_idx] = htons(data[i][j]);
            pkt_idx += 1;
        }
    }

    return pkt;
}

MusicMeta* ntohmm(uint16_t* pkt, MMHints *hints)
{
    MusicMeta *mm;

    // unpacks pkt size
    hints->pkt_size   =  ntohs(pkt[0]);
    
    // converts all packge to hots endianess
    for (int i = 0 ; i < hints->pkt_size; i++) pkt[i] = ntohs(pkt[i]);
    
    // finishes unpacking header
    hints->pkt_type   = (pkt[1] & 0b0000000000001111);
    hints->pkt_op     = (pkt[1] & 0b0000000011110000) >> 4;
    hints->pkt_status = (pkt[1] & 0b1111111100000000) >> 8;

    hints->pkt_filter = (pkt[2] & 0b0000000011111111);
    hints->pkt_numres = (pkt[2] & 0b1111111100000000) >> 8;

    mm = (MusicMeta *) calloc(hints->pkt_numres, sizeof(sizeof(MusicMeta)));

    // unpacks offset table section
    uint16_t *offsets = (uint16_t *) calloc(8 * hints->pkt_numres, sizeof(uint16_t));

    for (int i = 0; i < 8 * hints->pkt_numres; i++) offsets[i] = pkt[3 + i];

    // unpacks data section
    uint16_t off = 0;
    for (int i = 0; i < hints->pkt_numres; i++) {
        // secoff, idoff, rloff, ttoff, itoff, lgoff, ctoff, choff
        uint16_t fields_offsets[] = {
            offsets[off + 0],
            offsets[off + 1],
            offsets[off + 2],
            offsets[off + 3],
            offsets[off + 4],
            offsets[off + 5],
            offsets[off + 6],
            offsets[off + 7],
        };
        unsigned char *fields_values[] = {
            NULL,
            NULL,
            NULL,
            mm[i].title,
            mm[i].interpreter,
            mm[i].language,
            mm[i].category,
            mm[i].chorus
        };

        // moves pointer to next result offsets
        off += 8;

        // copies integer fields
        mm[i].id           = pkt[fields_offsets[0] + fields_offsets[1]];
        mm[i].release_year = pkt[fields_offsets[0] + fields_offsets[2]];

        // copies string fields
        for (uint16_t j = 3; j < 8; j++) read2char(fields_offsets[0] + fields_offsets[j], pkt, fields_values[j]);
    }

    return mm;
}