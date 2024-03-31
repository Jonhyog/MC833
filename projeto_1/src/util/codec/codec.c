#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "music.h"

#define BITPACK(A, B) ((A - 1) | (B - 1)) + 1

// pkt_type     : 02 bits
// pkt_op       : 02 bits
// pkt_status   : 02 bits
// pkt_size     : 10 bits 

// pkt_filter   : 08 bits
// pkt_numres   : 08 bits

// repete pkt_numres vezes
// pkt_secoff   : 16 bits (offset relativo ao inicio do pacote)
// pkt_idlen    : 16 bits (offset relativo ao inicio da seção )
// pkt_rllen    : 16 bits (offset relativo ao inicio da seção )
// pkt_ttlen    : 16 bits (offset relativo ao inicio da seção )

// pkt_itlen    : 16 bits (offset relativo ao inicio da seção )
// pkt_lglen    : 16 bits (offset relativo ao inicio da seção )

// pkt_ctlen    : 16 bits (offset relativo ao inicio da seção )
// pkt_chlen    : 16 bits (offset relativo ao inicio da seção )


typedef struct {
    uint16_t pkt_type;
    uint16_t pkt_op;
    uint16_t pkt_status;
    uint16_t pkt_size;
    uint16_t pkt_filter;
    uint16_t pkt_numres;
    uint16_t pkt_idlen;
    uint16_t pkt_rllen;
    uint16_t pkt_ttlen;
    uint16_t pkt_itlen;
    uint16_t pkt_lglen;
    uint16_t pkt_ctlen;
    uint16_t pkt_chlen;
} MSChdr;

// void print_bin(uint16_t n)
// {
//     for (int i = 15; i >= 0; --i) {
//         printf("%d", n >> i & 1);
//     }
//     printf("\n");
// }

uint16_t* htonmm(MusicMeta mm, uint16_t size, uint16_t op, uint16_t filter)
{   
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

    uint16_t pkt_size = 0;
    uint16_t offset_idx = 0;

    // header section
    uint16_t h1 = (op << 4);                     /* type == 0 && status == 0 */
    uint16_t h2 = filter | (size << 8);
    pkt_size += 2;

    // offset table section
    uint16_t *offsets = (uint16_t *) calloc(8 * size, sizeof(uint16_t));
    pkt_size += 8 * size;

    // data sections
    uint16_t **data = (uint16_t **) calloc(size, sizeof(uint16_t *));
    uint16_t *section_size = (uint16_t *) calloc(size, sizeof(uint16_t));

    for (int i = 0; i < size; i++) {
        uint16_t secoff;
        uint16_t idoff;
        uint16_t rloff;
        uint16_t ttoff;
        uint16_t itoff;
        uint16_t lgoff;
        uint16_t ctoff;
        uint16_t choff;

        // sets section offset
        secoff = pkt_size;

        // id && release_year
        idoff = 0;
        rloff = 1;
        section_size[i] += 2;

        // string fields
        printf("DEBUG: \n");
        printf("\tSEC_SIZE: %d - NEXT: %ld\n", section_size[i], (BITPACK(strlen(((char *) mm.title)       + 1), 16)) / 2);
        ttoff = section_size[i];
        section_size[i] += (BITPACK(strlen((char *) mm.title)       + 1, 16)) / 2;
        
        printf("\tSEC_SIZE: %d\n", section_size[i]);
        itoff = section_size[i];
        section_size[i] += (BITPACK(strlen((char *) mm.interpreter) + 1, 16)) / 2;

        printf("\tSEC_SIZE: %d\n", section_size[i]);
        lgoff = section_size[i];
        section_size[i] += (BITPACK(strlen((char *) mm.language)    + 1, 16)) / 2;

        printf("\tSEC_SIZE: %d\n", section_size[i]);
        ctoff = section_size[i];
        section_size[i] += (BITPACK(strlen((char *) mm.category)    + 1, 16)) / 2;

        printf("\tSEC_SIZE: %d\n", section_size[i]);
        choff = section_size[i];
        section_size[i] += (BITPACK(strlen((char *) mm.chorus)      + 1, 16)) / 2;

        printf("\tSEC_SIZE: %d\n", section_size[i]);
        // writes section offsets
        offsets[offset_idx + 0] = secoff;
        offsets[offset_idx + 1] = idoff;
        offsets[offset_idx + 2] = rloff;
        offsets[offset_idx + 3] = ttoff;
        offsets[offset_idx + 4] = itoff;
        offsets[offset_idx + 5] = lgoff;
        offsets[offset_idx + 6] = ctoff;
        offsets[offset_idx + 7] = choff;

        offset_idx += 8;

        // updates pkt size
        pkt_size += section_size[i];

        // allocates space for data
        data[i] = calloc(section_size[i], sizeof(uint16_t));

        // TODO: WRITE TO DATA
        data[i][idoff] = mm.id;
        data[i][rloff] = mm.release_year;

        // int idx = 0;
        // for (int j = ttoff; j < strlen(mm.title) / 2; j++) {
        //     // FIX-ME: Segmentation Fault if strlen is odd
        //     data[i][j] = mm.title[idx] | mm.title[idx + 1] << 8;
        //     idx += 2;
        // }

        int idx = 0;
        for (int j = 0; j < strlen((char *) mm.title); j += 2) {
            // FIX-ME: Segmentation Fault if strlen is odd
            data[i][ttoff + idx] = mm.title[j] | mm.title[j + 1] << 8;
            idx += 1;
        }

        idx = 0;
        for (int j = 0; j < strlen((char *) mm.interpreter); j += 2) {
            // FIX-ME: Segmentation Fault if strlen is odd
            data[i][itoff + idx] = mm.interpreter[j] | mm.interpreter[j + 1] << 8;
            idx += 1;
        }

        idx = 0;
        for (int j = 0; j < strlen((char *) mm.language); j += 2) {
            // FIX-ME: Segmentation Fault if strlen is odd
            data[i][lgoff + idx] = mm.language[j] | mm.language[j + 1] << 8;
            idx += 1;
        }

        idx = 0;
        for (int j = 0; j < strlen((char *) mm.category); j += 2) {
            // FIX-ME: Segmentation Fault if strlen is odd
            data[i][ctoff + idx] = mm.category[j] | mm.category[j + 1] << 8;
            idx += 1;
        }

        idx = 0;
        for (int j = 0; j < strlen((char *) mm.chorus); j += 2) {
            // FIX-ME: Segmentation Fault if strlen is odd
            data[i][choff + idx] = mm.chorus[j] | mm.chorus[j + 1] << 8;
            idx += 1;
        }
    }
    
    uint16_t *pkt = calloc(pkt_size, sizeof(u_int16_t));
    uint16_t pkt_idx = 2;

    // copies header
    pkt[0] = h1;
    pkt[1] = h2;

    // copies offset table
    for (int i = 0; i < 8 * size; i++) pkt[pkt_idx + i] = offsets[i];
    pkt_idx += 8 * size;

    // copies data to pkt
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < section_size[i]; j++) {
            pkt[pkt_idx] = data[i][j];
            pkt_idx += 1;
        }
    }

    printf("%d\n", pkt_idx);
    // FIX-ME: transform to network endianess

    return pkt;
}

MusicMeta* ntohmm(unsigned char* pkt)
{
    return NULL;
}