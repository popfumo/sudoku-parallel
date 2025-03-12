#include <stdint.h>
#pragma once
typedef struct {
    unsigned char base;
    unsigned char sidelength;
    short int n_zeros;
    unsigned char N;
    unsigned char **board;
    // These are not used in the base_solver implementation
    uint64_t *rbits;
    uint64_t *cbits;
    uint64_t *bbits;
} board_t;