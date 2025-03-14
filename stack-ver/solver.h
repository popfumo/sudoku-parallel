#include <stdint.h>
#pragma once
#define MAX_SIDELENGTH 100

typedef struct {
    unsigned char base;
    unsigned char sidelength;
    short int n_zeros;
    unsigned char N;
    unsigned char (*board)[MAX_SIDELENGTH]; // Changed type to pointer to array
    uint64_t *rbits;
    uint64_t *cbits;
    uint64_t *bbits;
} board_t;