#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <omp.h>
#include <stdint.h>  
#include "verify.h"
#include "solver.h"

typedef struct {
    unsigned char x; 
    unsigned char y; 
} ua_t; 

void print_board(board_t *board_data, int sidelength) {
    unsigned char **board = board_data->board;
    unsigned char base = board_data->base;
    
    for(int i = 0; i < sidelength; i++) {
        if (i > 0 && i % base == 0) {
            for (int k = 0; k < sidelength * 2 + base - 1; k++) {
                printf("-");
            }
            printf("\n");
        }
        
        for(int j = 0; j < sidelength; j++) {
            printf("%d", board[i][j]);
            if (j < sidelength - 1) {
                if ((j + 1) % base == 0) {
                    printf(" | ");
                } else {
                    printf(" ");
                }
            }
        }
        printf("\n");
    }
}

void print_ua(ua_t *ua, int n_zeros) {
    for(int i = 0; i < n_zeros; i++) {
        printf("ua[%d] x: %d y: %d\n", i, ua[i].x, ua[i].y);
    }
}

void bitmask_init(board_t *board) {
    unsigned char base = board->base;
    unsigned char sidelength = board->sidelength;
    board->rbits = calloc(sidelength, sizeof(uint64_t));
    board->cbits = calloc(sidelength, sizeof(uint64_t));
    board->bbits = calloc(sidelength, sizeof(uint64_t));
    for(int i = 0; i < sidelength; i++) {
        for(int j = 0; j < sidelength; j++) {
            unsigned char curr_val = board->board[i][j];
            // Checking if the value is not 0, if it is not 0, update the bitmasks
            if(curr_val != 0) {
                // Block index calculation
                unsigned char block_idx = (i / base) * base + (j / base);
                // Handwavy, setting the bits for the row and column block
                // Example: we are at the first row (i=0) and curr_val = 3
                // then we need to set that bit for that i by applying an OR operation
                // to the current i with a 1 that is shifted 3 bits to the left 1 << 3 = 1000
                board->rbits[i] |= ((uint64_t)1 << curr_val);
                board->cbits[j] |= ((uint64_t)1 << curr_val);
                board->bbits[block_idx] |= ((uint64_t)1 << curr_val);
            }
        }
    }
}

ua_t *board_init(int board_size, board_t *board) {
    FILE *file = NULL;
    if(board_size == 36){
        file = fopen("boards/board_36x36.dat", "r");
    }
    if(board_size == 25){
        file = fopen("boards/board_25x25.dat", "r");
    }
    if(board_size == 64){
        file = fopen("boards/board_64x64.dat", "r");
    }

    if(file == NULL) {
        printf("File not found\n");
        return NULL;
    }
 
    unsigned char base = 0;
    unsigned char sidelength = 0;
    if(fread(&base, sizeof(unsigned char), 1, file) != 1) {
        printf("Error reading base\n");
        fclose(file);
        return NULL;
    }
    if(fread(&sidelength, sizeof(unsigned char), 1, file) != 1) {
        printf("Error reading side length\n");
        fclose(file);
        return NULL;
    }
    
    board->base = base;
    board->sidelength = sidelength;
    board->board = malloc(sidelength * sizeof(unsigned char *));
    board->N = sidelength * sidelength;
    board->n_zeros = 0;
    ua_t *ua = calloc(sidelength * sidelength, sizeof(ua_t));
    int curr = 0;
    printf("Base: %d\n", base);
    printf("Sidelength: %d\n", sidelength);
    for(int i = 0; i < sidelength; i++) {
        board->board[i] = malloc(sidelength * sizeof(unsigned char));
        for(int j = 0; j < sidelength; j++) {
            if(fread(&board->board[i][j], sizeof(unsigned char), 1, file) != 1) {
                printf("Error reading board data!\n");
                fclose(file);
                return NULL;
            }
            if(board->board[i][j] == 0) {
                board->n_zeros++;
                ua[curr].x = i; 
                ua[curr].y = j;
                curr++;
            }
        }
    }
    bitmask_init(board);
    fclose(file); 
    return ua;
   
    

}

void bit_update(board_t *board, int row, int column, int value, bool add) {
    int block_idx = (row / board->base) * board->base + (column / board->base);
    uint64_t mask = ((uint64_t)1 << value);
    if(add) {
        // Add the value to the bitmasks using the OR operator
        board->rbits[row] |= mask;
        board->cbits[column] |= mask;
        board->bbits[block_idx] |= mask;
    } else {
        // Remove the value from the bitmasks using the NOT operator
        // Example: we have a row bitmask of 1111 and we want to remove the value 2
        // we need to create a mask of 1011 and apply an AND operation to the row bitmask
        board->rbits[row] &= ~mask;
        board->cbits[column] &= ~mask;
        board->bbits[block_idx] &= ~mask;
    }
}

bool validate(board_t *board, int row, int column, int value) {
    
    // can't this be done by row + column / base?
    int block_idx = (row / board->base) * board->base + (column / board->base);
    uint64_t mask = ((uint64_t)1 << value);
    return !(
        (board->rbits[row] & mask) || 
        (board->cbits[column] & mask) || 
        (board->bbits[block_idx] & mask)
    );
}

bool solver(ua_t *ua, board_t *board, short int zeroes) {
    if(zeroes == 0) {
        return true;
    }
    ua_t index = ua[zeroes-1];
    int row = index.x;
    int column = index.y;
    for(int i = 1; i <= board->sidelength; i++) {
        if(validate(board, row, column, i)) {
            // Place value after validation instead 
            board->board[index.x][index.y] = i;
            bit_update(board, row, column, i, true);
            if(solver(ua, board, zeroes - 1)) {
                return true;
            }
            board->board[index.x][index.y] = 0;
            bit_update(board, row, column, i, false);
        }
        

    }
    return false;
}

int main(int argc, char *argv[]) {

    if(argc != 3) {
        printf("Usage: <board_size>\n");
        printf("board_size: 25, 36, 64\n");
        return 1;
    }
    int board_size = atoi(argv[1]);

    board_t *board = malloc(sizeof(board_t));
    ua_t *ua = board_init(board_size, board);
    if (board == NULL) {
        printf("Error initializing board\n");
        return 1;
    }
    printf("no zeros: %d\n", board->n_zeros);
    double time = omp_get_wtime();
    print_board(board, board->sidelength);

    solver(ua, board, board->n_zeros);

    
    time = omp_get_wtime() - time;
    for(int i = 0; i < board->sidelength; i++) {
        printf("_____");
    }
    printf("\n");
    printf("Solved board\n");
    print_board(board, board->sidelength);
    printf("Time taken: %f seconds \n", time);
    printf("Veryfing board...\n");
    if(!(verify(board))) {
        printf("Invalid solution\n");
    }
    else {
        printf("Valid solution\n");
    }
    free(board->rbits);
    free(board->cbits);
    free(board->bbits);
    free(ua);
    for(int i = 0; i < board->sidelength; i++) {
        free(board->board[i]);
    }
    free(board->board);
    free(board);
    return 0;
}