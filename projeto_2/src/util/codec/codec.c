#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <netinet/in.h>

#include "music.h"
#include "codec.h"

#define BITPACK(A, B) ((A - 1) | (B - 1)) + 1
#define MIN(A, B) A <= B ? A : B

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
    uint16_t h1 = hints->pkt_type | (hints->pkt_op << 4) | (hints->pkt_status << 8);
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
        data[i][fields_offsets[1]] = mm[i].id;
        data[i][fields_offsets[2]] = mm[i].release_year;

        // writes string fields
        for (int j = 3; j < 8; j++) write2int16(fields_offsets[j], fields_values[j], data[i]);
    }
    
    hints->pkt_size = pkt_size * sizeof(uint16_t);
    uint16_t *pkt = calloc(pkt_size, sizeof(uint16_t));
    uint16_t pkt_idx = 3;

    // copies header
    pkt[0] = htons(hints->pkt_size);
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

    mm = (MusicMeta *) calloc(hints->pkt_numres, sizeof(MusicMeta));

    // unpacks offset table section
    uint16_t *offsets = (uint16_t *) calloc(8 * hints->pkt_numres, sizeof(uint16_t));

    for (int i = 0; i < 8 * hints->pkt_numres; i++) offsets[i] = pkt[3 + i];

    // unpacks data section
    uint16_t off = 0;
    for (int i = 0; i < hints->pkt_numres; i++) {
        // secoff, idoff, rloff, ttoff, itoff, lgoff, ctoff, choff
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

        // copies integer fields
        mm[i].id           = pkt[offsets[off] + offsets[off + 1]];
        mm[i].release_year = pkt[offsets[off] + offsets[off + 2]];

        // copies string fields
        for (int j = 3; j < 8; j++) read2char(offsets[off] + offsets[off + j], pkt, fields_values[j]);

        // moves pointer to next result offsets
        off += 8;
    }

    return mm;
}

int file_size(FILE *file)
{
    int size;

	fseek(file, 0L, SEEK_END);
	size = ftell(file);
	rewind(file);

	return size;
}

int read_file(FILE *fd, uint16_t *buff)
{
    // FIXME: increse maximum music size
    int size = fread(buff, sizeof(uint16_t), MAX_MUSIC_SIZE, fd);
    fclose(fd);

    return size;
}

uint16_t** htonmd(FILE *md, MMHints *hints, int *frags)
{
    uint16_t *buff = (uint16_t *) malloc(MAX_MUSIC_SIZE);
    uint16_t **pkt;
    int total_size, num_frags;
    int file_idx;

    // Reads file and calculates num of
    // fragments to send over udp
    total_size = read_file(md, buff);
    int temp_size = total_size;
    num_frags = (total_size / (UDP_SIZE - 5)) + ((total_size % (UDP_SIZE - 5)) != 0);

    // Allocates memory for all fragments (with headers included)
    pkt = (uint16_t **) calloc(num_frags, sizeof(uint16_t *));
    for (int i = 0; i < num_frags; i++) {
        // size(pkt[i]) = 
        //              = (pkt_size + h1 + h2 + h3) + FRAG_SIZE
        //              = 6 + FRAG_SIZE
        // We use FRAG_SIZE / 2 because FRAG_SIZE is in bytes and uint16 == 2 bytes
        pkt[i] = (uint16_t *) calloc(UDP_SIZE, sizeof(uint16_t));
    }

    // Assembles udp pkts
    file_idx = 0;
    for (int i = 0; i < num_frags; i++) {
        pkt[i][0] = MIN(total_size + 5, UDP_SIZE);
        pkt[i][1] = hints->pkt_type | (hints->pkt_op << 4) | (hints->pkt_status << 8);
        pkt[i][2] = hints->pkt_filter | (hints->pkt_numres << 8);
        pkt[i][3] = i;
        pkt[i][4] = num_frags;

        total_size -= pkt[i][0];

        // Copies music data to pkt
        for (int j = 0; j < pkt[i][0] - 5; j++) {
            pkt[i][j + 5] = buff[file_idx++];
        }
    }

    // FIX-ME: Does not copy end of song
    printf("server: copied %d uint_16 from %d frags\n", file_idx, num_frags);

    // sanity test
    uint16_t *test = (uint16_t *) calloc(MAX_MUSIC_SIZE, sizeof(uint16_t));
    int test_idx = 0;
    for (int i = 0; i < num_frags; i++) {
        // for (int j = 0; j < pkt[i][0] - 5; j++) {
        //     // buff[file_idx++] = pkt[i][j + 5];
        //     test[(pkt[i][3] * (UDP_SIZE - 5)) + j] = pkt[i][j + 5];
        // }

        // FIX-ME: Does not order pkts
        for (int j = 0; j < pkt[i][0] - 5; j++) {
            test[test_idx] = pkt[i][j + 5];
            test_idx++;
        }
    }

    int diffs = 0;
    for (int i = 0; i < temp_size; i++) {
        if (test[i] != buff[i]) {
            diffs++;
        }
    }

    printf("server: found %d diffs in %d bytes!\n", diffs, 2 * temp_size);

    // Rewrites pkt with network byte order
    for (int i = 0; i < num_frags; i++) {
        for (int j = 0; j < UDP_SIZE; j++) {
            pkt[i][j] = htons(pkt[i][j]);
        }
    }

    *(frags) = num_frags;
    return pkt;
}

uint16_t* ntohmd(uint16_t** pkt, MMHints *hints, int count)
{
    uint16_t *buff = (uint16_t *) calloc(MAX_MUSIC_SIZE, sizeof(uint16_t));
    uint16_t num_frags = ntohs(pkt[0][4]);
    // uint16_t file_idx;

    // Rewrites pkt with host byte order
    for (int i = 0; i < num_frags; i++) {
        for (int j = 0; j < UDP_SIZE; j++) {
            pkt[i][j] = ntohs(pkt[i][j]);
        }
    }

    // finishes unpacking header
    hints->pkt_type   = (pkt[0][1] & 0b0000000000001111);
    hints->pkt_op     = (pkt[0][1] & 0b0000000011110000) >> 4;
    hints->pkt_status = (pkt[0][1] & 0b1111111100000000) >> 8;

    hints->pkt_filter = (pkt[0][2] & 0b0000000011111111);
    hints->pkt_numres = (pkt[0][2] & 0b1111111100000000) >> 8;

    // TODO: Reorder pkts

    // Copies Music Data
    // file_idx = 0;
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < pkt[i][0] - 5; j++) {
            // buff[file_idx++] = pkt[i][j + 5];
            buff[(pkt[i][3] * (UDP_SIZE - 5)) + j] = pkt[i][j + 5];
        }
    }

    return buff;
}

void recvall(int fd, uint16_t *buff, int buff_size, int flags)
{
    int size = 0;
    uint16_t pkt_size = -1;

    while (1) {
        if ((size += recv(fd, buff, buff_size - 1, flags)) == -1) {
            perror("recv");
            exit(1);
	    }

        if (size > 1 && pkt_size != -1) {
            pkt_size = ntohs(buff[0]);
            // printf("Expecting %d bytes\n", pkt_size);
        }

        if (size == pkt_size) {
            break;
        }

        // FIX-ME: should timeout if receives something
        // but takes too long (e.g. > 10s) to recv all pkt
    }
}

int sendall(int fd, uint16_t *buff, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len;  // how many we have left to send
    int n;

    while(total < *len) {
        n = send(fd, buff + total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here

    return n == -1 ? -1 : 0; // return -1 on failure, 0 on success
}