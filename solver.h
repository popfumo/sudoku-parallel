#include <stdint.h>
#pragma once
#define MAX_SIDELENGTH 64

typedef struct {
    unsigned char base;
    unsigned char sidelength;
    short int n_zeros;
    unsigned char (*board)[MAX_SIDELENGTH];
    int64_t *rbits;
    int64_t *cbits;
    int64_t *bbits;

} board_t;

typedef struct {
    unsigned char x; 
    unsigned char y; 
} ua_t; 


int board_init(int board_size, board_t *board, ua_t *ua);

bool solver(ua_t *ua, board_t *board, short int zeroes, int cutoff);