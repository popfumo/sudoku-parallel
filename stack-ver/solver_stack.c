#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <omp.h>
#include <stdint.h>  
#include "verify.h"
#include "solver.h"

// Define maximum sizes based on largest possible board (100x100)
#define MAX_SIDELENGTH 100
#define MAX_CELLS 10000

volatile bool global_flag = false;

typedef struct {
    unsigned char x; 
    unsigned char y; 
} ua_t; 
int task_count = 0;

void print_board(board_t *board_data, int sidelength) {
    unsigned char base = board_data->base;    
    for(int i = 0; i < sidelength; i++) {
        if (i > 0 && i % base == 0) {
            for (int k = 0; k < sidelength * 2 + base - 1; k++) {
                printf("-");
            }
            printf("\n");
        }
        
        for(int j = 0; j < sidelength; j++) {
            printf("%d", board_data->board[i][j]);
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

void bitmask_init(board_t *board) {
    unsigned char base = board->base;
    unsigned char sidelength = board->sidelength;
    for(int i = 0; i < sidelength; i++) {
        for(int j = 0; j < sidelength; j++) {
            unsigned char curr_val = board->board[i][j];
            // Checking if the value is not 0, if it is not 0, update the bitmasks
            if(curr_val != 0) {
                // Block index calculation
                unsigned char block_idx = (i / base) * base + (j / base);
                // Setting the bits for the row, column and block
                board->rbits[i] |= ((uint64_t)1 << curr_val);
                board->cbits[j] |= ((uint64_t)1 << curr_val);
                board->bbits[block_idx] |= ((uint64_t)1 << curr_val);
            }
        }
    }
}

int board_init(int board_size, board_t *board, ua_t *ua) {
    FILE *file = NULL;
    if(board_size == 36){
        file = fopen("../boards/board_36x36.dat", "r");
    }
    if(board_size == 25){
        file = fopen("../boards/board_25x25.dat", "r");
    }
    if(board_size == 64){
        file = fopen("../boards/board_64x64.dat", "r");
    }
    if(board_size == 100){
        file = fopen("../boards/board_100x100_2.dat", "r");
    }
    if(file == NULL) {
        printf("File not found\n");
        return 0;
    }
 
    unsigned char base = 0;
    unsigned char sidelength = 0;
    if(fread(&base, sizeof(unsigned char), 1, file) != 1) {
        printf("Error reading base\n");
        fclose(file);
        return 0;
    }
    if(fread(&sidelength, sizeof(unsigned char), 1, file) != 1) {
        printf("Error reading side length\n");
        fclose(file);
        return 0;
    }
    
    board->base = base;
    board->sidelength = sidelength;
    board->N = sidelength * sidelength;
    board->n_zeros = 0;
    
    // Initialize bitmasks to zero
    memset(board->rbits, 0, sizeof(uint64_t) * MAX_SIDELENGTH);
    memset(board->cbits, 0, sizeof(uint64_t) * MAX_SIDELENGTH);
    memset(board->bbits, 0, sizeof(uint64_t) * MAX_SIDELENGTH);
    
    int curr = 0;
    printf("Base: %d\n", base);
    printf("Sidelength: %d\n", sidelength);
    for(int i = 0; i < sidelength; i++) {
        for(int j = 0; j < sidelength; j++) {
            if(fread(&board->board[i][j], sizeof(unsigned char), 1, file) != 1) {
                printf("Error reading board data!\n");
                fclose(file);
                return 0;
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
    return 1;
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
        board->rbits[row] &= ~mask;
        board->cbits[column] &= ~mask;
        board->bbits[block_idx] &= ~mask;
    }
}

bool validate(board_t *board, int row, int column, int value) {
    int block_idx = (row / board->base) * board->base + (column / board->base);
    uint64_t mask = ((uint64_t)1 << value);
    return !(
        (board->rbits[row] & mask) || 
        (board->cbits[column] & mask) || 
        (board->bbits[block_idx] & mask)
    );
}

bool solver(ua_t *ua, board_t *board, short int zeroes) {
    #pragma omp flush(global_flag)
    if(global_flag) {
        return false;
    }
    
    if(zeroes == 0) {
        #pragma omp critical
        {
            if (!global_flag) {
                global_flag = true;
                printf("No zeroes left, checking solution\n");
                if(verify(*board))
                {
                    print_board(board, board->sidelength);
                    printf("Valid solution\n");
                }
                else
                {
                    printf("Invalid solution\n");
                }
                
            }
        }
        return true;
    }
    
    ua_t index = ua[zeroes-1];
    int row = index.x;
    int column = index.y;

    // Only create tasks at higher levels of recursion
    bool task_level = zeroes > board->n_zeros - 20; 

    if(task_level) {
        bool local_solved = false;
        
        for(int i = 1; i <= board->sidelength; i++) {
            #pragma omp flush(global_flag)
            //if(global_flag) break;
            
            if(validate(board, row, column, i)) {
                #pragma omp task shared(global_flag) firstprivate(i)
                {
                    // Create a local copy of the board for this task
                    #pragma omp atomic
                    task_count++;
                    board_t task_board;
                    unsigned char task_board_array[MAX_SIDELENGTH][MAX_SIDELENGTH];
                    uint64_t task_rbits[MAX_SIDELENGTH];
                    uint64_t task_cbits[MAX_SIDELENGTH];
                    uint64_t task_bbits[MAX_SIDELENGTH];                     // Initialize the local copy
                    memcpy(&task_board, board, sizeof(board_t));
                    task_board.board = task_board_array;
                    task_board.rbits = task_rbits;
                    task_board.cbits = task_cbits;
                    task_board.bbits = task_bbits;
                    
                    // Copy the board contents
                    memcpy(task_board_array, board->board, sizeof(unsigned char) * MAX_SIDELENGTH * MAX_SIDELENGTH);
                    memcpy(task_rbits, board->rbits, sizeof(uint64_t) * MAX_SIDELENGTH);
                    memcpy(task_cbits, board->cbits, sizeof(uint64_t) * MAX_SIDELENGTH);
                    memcpy(task_bbits, board->bbits, sizeof(uint64_t) * MAX_SIDELENGTH);
                    
                    // Place the value
                    task_board.board[row][column] = i;
                    bit_update(&task_board, row, column, i, true);
                    
                    // Recursively solve
                    if(solver(ua, &task_board, zeroes - 1)) {
                            #pragma omp flush(global_flag)
                            if (!global_flag) {
                                global_flag = true;
                                local_solved = true;
                                // Copy solution back to the original board
                                memcpy(board->board, task_board.board, sizeof(unsigned char) * MAX_SIDELENGTH * MAX_SIDELENGTH);
                                memcpy(board->rbits, task_board.rbits, sizeof(uint64_t) * MAX_SIDELENGTH);
                                memcpy(board->cbits, task_board.cbits, sizeof(uint64_t) * MAX_SIDELENGTH);
                                memcpy(board->bbits, task_board.bbits, sizeof(uint64_t) * MAX_SIDELENGTH);
                            }
                        
                    }
                } 
            }
        }
        
        #pragma omp taskwait
        return local_solved || global_flag;
    }
    else {
        task_count++;
        for(int i = 1; i <= board->sidelength; i++) {
            #pragma omp flush(global_flag)
            if(global_flag) return false;
            
            if(validate(board, row, column, i)) {
                board->board[row][column] = i;
                bit_update(board, row, column, i, true);
                
                if(solver(ua, board, zeroes - 1)) {
                    return true;
                }
                
                board->board[row][column] = 0;
                bit_update(board, row, column, i, false);
            }
        }
        return false;
    }
}

int main(int argc, char *argv[]) {
    if(argc != 3) {
        printf("Usage: %s <board_size> <option>\n", argv[0]);
        printf("board_size: 25, 36, 64, 100\n");
        return 1;
    }
    int board_size = atoi(argv[1]);
    int nthreads = atoi(argv[2]);

    board_t board;
    ua_t ua[MAX_CELLS];
    unsigned char board_array[MAX_SIDELENGTH][MAX_SIDELENGTH];
    uint64_t rbits[MAX_SIDELENGTH];
    uint64_t cbits[MAX_SIDELENGTH];
    uint64_t bbits[MAX_SIDELENGTH];
    

    board.board = (unsigned char **)board_array;
    board.rbits = rbits;
    board.cbits = cbits;
    board.bbits = bbits;

    if (!board_init(board_size, &board, ua)) {
        printf("Error initializing board\n");
        return 1;
    }
    
    printf("no zeros: %d\n", board.n_zeros);
    double time = omp_get_wtime();
    print_board(&board, board.sidelength);
    omp_set_nested(1);
    #pragma omp parallel num_threads(nthreads)
    {
        #pragma omp single
        {
            solver(ua, &board, board.n_zeros);
        }
        #pragma omp taskwait
    }
    
    time = omp_get_wtime() - time;
    for(int i = 0; i < board.sidelength; i++) {
        printf("___");
    }
    printf("\n");

    printf("Time taken: %f seconds \n", time);
    printf("Task count: %d\n", task_count);
    return 0;
}