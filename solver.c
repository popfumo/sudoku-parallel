#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <omp.h>
#include <stdint.h>  
#include "verify.h"
#include "solver.h"

// Defining the max size of the board
#define MAX_SIDELENGTH 64
#define MAX_CELLS 4096

// Switch to 1 for performance testing
#define PERFORMANCE 0

// Switch to 1 if you want to allocate memory on the heap instead of the stack
#define HEAP_ALLOCATION 0


// Don't touch this
volatile bool global_flag = false;

/**
 * @brief Prints the Sudoku board in a formatted manner.
 *
 * This function takes a Sudoku board and its side length as input and prints
 * the board to the console in a human-readable format.
 * 
 * @param board_data Pointer to the board_t structure containing the Sudoku board data.
 *                   
 * @param sidelength The length of one side of the Sudoku board.
 *
 * It uses the base value from the board_t structure to determine the size of sub-grids 
 * and prints horizontal and vertical separators accordingly.
 */
void print_board(board_t *board_data, int sidelength) {
    unsigned char base = board_data->base;    
    for (int i = 0; i < sidelength; i++) {
        if (i > 0 && i % base == 0) {
            for (int k = 0; k < sidelength * 3 + base ; k++) {
                printf("-");
            }
            printf("\n");
        }
        
        for (int j = 0; j < sidelength; j++) {
            if (j % base == 0 && j > 0) {
                printf(" |");
            }
            // Expand the minimum width so it looks nice
            printf(" %2d", board_data->board[i][j]);
        }
        printf("\n");
    }
}

/**
 * @brief Initializes the bitmask arrays for a Sudoku board.
 *
 * This function sets up the bitmask arrays for rows, columns, and blocks respectively, based on the current state 
 * of the Sudoku board. Each bit in these arrays represents whether a number is present in the corresponding row, column, or block.
 *
 * @param board A pointer to the Sudoku board structure.
 * 
 * The function iterates through each cell of the Sudoku board. If the cell 
 * contains a non-zero value, it updates the corresponding bitmask arrays 
 * by setting the bit corresponding to the value in the appropriate row, column, and block.
 *
 * Example:
 * - If the value at board->board[0][2] is 3, the function will set the 
 *   3rd bit in rbits[0], cbits[2], and the bitmask for the block 
 *   containing the cell. 
 *
 * @note The bitmask operations use 64-bit integers to represent the presence 
 *   of numbers in rows, columns, and blocks. This means that the max size of
 *   the Sudoku board is limited to 64x64.
 */
static inline void bitmask_init(board_t *board) {
    unsigned char base = board->base;
    unsigned char sidelength = board->sidelength;
    for(int i = 0; i < sidelength; i++) {
        for(int j = 0; j < sidelength; j++) {
            unsigned char curr_val = board->board[i][j];
            // Checking if the value is not 0, if it is not 0, update the bitmasks
            if(curr_val != 0) {
                int curr_block = (i / base) * base + (j / base);
                board->rbits[i] |= ((int64_t)1 << curr_val);
                board->cbits[j] |= ((int64_t)1 << curr_val);
                board->bbits[curr_block] |= ((int64_t)1 << curr_val);
            }
        }
    }
}

/**
 * @brief Initializes the Sudoku board by reading data from a file based on the board size.
 * 
 * This function reads a pre-defined Sudoku board from a file, initializes the board structure,
 * and populates the unassigned array with the coordinates of unassigned cells (cells with value 0).
 * 
 * @param board_size The size of the Sudoku board.
 * @param board Pointer to the board_t structure to be initialized.
 * @param ua Pointer to the array of unassigned cells.
 * 
 * @return Returns 1 on successful initialization, or 0 if an error occurs.
 * 
 * @note The function also initializes bitmask arrays and calculates the number of unassigned cells.
 * 
 * @warning Ensure that the file paths and formats match the expected structure. The function
 *          will return 0 and print an error message if the file is missing or corrupted.
 */
inline int board_init(int board_size, board_t *board, ua_t *ua) {
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
    board->n_zeros = 0;
    
    memset(board->rbits, 0, sizeof(int64_t) * MAX_SIDELENGTH);
    memset(board->cbits, 0, sizeof(int64_t) * MAX_SIDELENGTH);
    memset(board->bbits, 0, sizeof(int64_t) * MAX_SIDELENGTH);
    
    int curr = 0;
    // Place the values on the board
    for(int i = 0; i < sidelength; i++) {
        for(int j = 0; j < sidelength; j++) {
            if(fread(&board->board[i][j], sizeof(unsigned char), 1, file) != 1) {
                printf("Error reading board data\n");
                fclose(file);
                return 0;
            }
            // If the value is zero, its unassigned which means that we add it to the unassigned array.
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

/**
 * @brief Updates the bitmask representation of the Sudoku board for a specific cell.
 *
 * This function modifies the bitmasks associated with rows, columns, and blocks
 * in the Sudoku board to either add or remove a specific value. The bitmasks are
 * used to efficiently track which numbers are present in each row, column, and block.
 *
 * @param board Pointer to the Sudoku board structure.
 * @param row The row index of the cell to update.
 * @param column The column index of the cell to update.
 * @param value The value to add or remove from the bitmasks.
 * @param add A boolean flag indicating the operation to perform (true for add, false for remove).
 *
 */
static inline void bit_update(board_t *board, int row, int column, int value, bool add) {
    int curr_block = (row / board->base) * board->base + (column / board->base);
    int64_t mask = ((int64_t)1 << value);
    if(add) {
        // Add the value to the bitmasks using the OR operator
        board->rbits[row] |= mask;
        board->cbits[column] |= mask;
        board->bbits[curr_block] |= mask;
    } else {
        // Remove the value from the bitmasks using the NOT operator
        board->rbits[row] &= ~mask;
        board->cbits[column] &= ~mask;
        board->bbits[curr_block] &= ~mask;
    }
}

/**
 * @brief Solves a Sudoku puzzle using a parallel backtracking algorithm.
 *
 * Attempts to solve a Sudoku puzzle represented by the given board.
 * It uses OpenMP tasks for parallel execution when the number of remaining zeroes
 * exceeds a specified cutoff. The function updates the board in place and verifies
 * the solution upon completion.
 *
 * @param ua An array of coordinates (ua_t) representing the positions of zeroes in the board.
 * @param board A pointer to the Sudoku board structure (board_t) containing the puzzle state.
 * @param zeroes The number of zeroes (empty cells) remaining in the board.
 * @param cutoff The threshold for switching between parallel and sequential execution.
 * @return true if a solution is found, false otherwise.
 *
 * @note Memory allocation for board copies can be configured to use either heap or stack, depending on the HEAP_ALLOCATION macro.
 * 
 */
bool solver(ua_t *ua, board_t *board, short int zeroes, int cutoff) {
    // Check if someone found the solution, flush so we read the actual value from memory instead of private cache
    #pragma omp flush(global_flag)
    if(global_flag) {
        return false;
    }
    if(zeroes == 0) {
        // No zeroes left, set global to true and verify the solution
        #pragma omp critical
        {
            #pragma omp flush(global_flag)
            if (!global_flag) {
                global_flag = true;
                if(verify(*board))
                {   
                    #if PERFORMANCE

                    #else
                    printf("\n");
                    printf("------------------------------------------------------\n");
                    print_board(board, board->sidelength);
                    printf("Valid solution\n");

                    #endif
                }
                else
                {
                    printf("Invalid solution\n");
                }
                
            }
        }
        return true;
    }
    // Find the current zero index
    ua_t index = ua[zeroes-1];
    int row = index.x;
    int column = index.y;
    // Check if we are at the parallel cutoff, begin serial execution if thats the case 
    bool task_level = zeroes > (board->n_zeros-cutoff);
    if(task_level) {
        for(int i = 1; i <= board->sidelength; i++) {
            // Find the current subgrid and create the mask 
            int curr_block = (row / board->base) * board->base + (column / board->base);
            int64_t mask = ((int64_t)1 << i);
            // Check that the value is not already in the row, column or block
            if(!((board->rbits[row] & mask) || (board->cbits[column] & mask) || (board->bbits[curr_block] & mask))) {
                // Placement is valid, create a task with a board copy
                #pragma omp task firstprivate(i, zeroes)
                {
                    // Dynamically allocate memory if HEAP_ALLOCATION is set to 1 
                    #if HEAP_ALLOCATION
                    board_t task_board;
                    unsigned char (*task_board_array)[MAX_SIDELENGTH] = malloc(sizeof(unsigned char) * MAX_SIDELENGTH * MAX_SIDELENGTH);
                    int64_t *task_rbits = malloc(sizeof(int64_t) * MAX_SIDELENGTH);
                    int64_t *task_cbits = malloc(sizeof(int64_t) * MAX_SIDELENGTH);
                    int64_t *task_bbits = malloc(sizeof(int64_t) * MAX_SIDELENGTH);
                    #else
                    // Allocated on the stack
                    board_t task_board;
                    unsigned char task_board_array[MAX_SIDELENGTH][MAX_SIDELENGTH];
                    int64_t task_rbits[MAX_SIDELENGTH];
                    int64_t task_cbits[MAX_SIDELENGTH];
                    int64_t task_bbits[MAX_SIDELENGTH];
                    #endif
                    
                    // Copy the board and the bitmasks
                    memcpy(&task_board, board, sizeof(board_t));
                    task_board.board = task_board_array;
                    task_board.rbits = task_rbits;
                    task_board.cbits = task_cbits;
                    task_board.bbits = task_bbits;
                    memcpy(task_board_array, board->board, sizeof(unsigned char) * MAX_SIDELENGTH * MAX_SIDELENGTH);
                    memcpy(task_rbits, board->rbits, sizeof(int64_t) * MAX_SIDELENGTH);
                    memcpy(task_cbits, board->cbits, sizeof(int64_t) * MAX_SIDELENGTH);
                    memcpy(task_bbits, board->bbits, sizeof(int64_t) * MAX_SIDELENGTH);

                    // Place the value on the board and then update the bitmask 
                    task_board.board[row][column] = i;
                    bit_update(&task_board, row, column, i, true);
                    
                    // Recursive call 
                    if(solver(ua, &task_board, zeroes - 1, cutoff)) {
                        memcpy(board->board, task_board.board, sizeof(unsigned char) * MAX_SIDELENGTH * MAX_SIDELENGTH);
                        memcpy(board->rbits, task_board.rbits, sizeof(int64_t) * MAX_SIDELENGTH);
                        memcpy(board->cbits, task_board.cbits, sizeof(int64_t) * MAX_SIDELENGTH);
                        memcpy(board->bbits, task_board.bbits, sizeof(int64_t) * MAX_SIDELENGTH);                        
                    }
                    
                    #if HEAP_ALLOCATION
                    // Free the allocated copy
                    free(task_board_array);
                    free(task_rbits);
                    free(task_cbits);
                    free(task_bbits);
                    #endif
                } 
            }
        }
        
        #pragma omp taskwait
        return false;
    }
    // Sequential execution, same stuff but without the annoying copying
    else {
        for(int i = 1; i <= board->sidelength; i++) {

            int curr_block = (row / board->base) * board->base + (column / board->base);
            int64_t mask = ((int64_t)1 << i);
            if(!((board->rbits[row] & mask) || (board->cbits[column] & mask) || (board->bbits[curr_block] & mask))) {
                board->board[row][column] = i;
                bit_update(board, row, column, i, true);
                
                if(solver(ua, board, zeroes - 1, cutoff)) {
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
    if(argc != 4) {
        printf("Usage: %s <board_size> <threads> <cutoff>\n", argv[0]);
        printf("board_size: 25, 36, 64\n");
        printf("Recommended cutoff for board sizes: 25x25 - any, 36x36 - 5, 64x64 - 100 (max 500)\n");
        return 1;
    }


    #if PERFORMANCE
    int size_array[3] = {25, 36, 64};
    int thread_array[6] = {1, 2, 4, 6, 8, 12};
    int cutoff_array[9] = {1, 5, 10, 20, 30, 40, 50, 70, 100};
    for(int i = 0; i < 3; i++){
        int board_size = size_array[i];
        if(board_size == 64) {
            cutoff_array[0] = 50;
            cutoff_array[1] = 75;
            cutoff_array[2] = 100;
            cutoff_array[3] = 150;
            cutoff_array[4] = 200;
            cutoff_array[5] = 250;
            cutoff_array[6] = 300;
            cutoff_array[7] = 400;
            cutoff_array[8] = 500;
        }
        for(int j = 0; j < 6; j++){
            int nthreads = thread_array[j];
            for(int k = 0; k < 9; k++){
                int cutoff = cutoff_array[k];
                double avg_time = 0;
                for(int m = 0; m < 5; m++){
                    board_t board;
                    ua_t ua[MAX_CELLS];
                    unsigned char board_array[MAX_SIDELENGTH][MAX_SIDELENGTH];
                    int64_t rbits[MAX_SIDELENGTH];
                    int64_t cbits[MAX_SIDELENGTH];
                    int64_t bbits[MAX_SIDELENGTH];
                    
            
                    board.board = (unsigned char (*)[MAX_SIDELENGTH])board_array;
                    board.rbits = rbits;
                    board.cbits = cbits;
                    board.bbits = bbits;
            
                    if (!board_init(board_size, &board, ua)) {
                        printf("Error initializing board\n");
                        return 1;
                    }
                    double time1 = omp_get_wtime();
                    global_flag = false;
                    #pragma omp parallel num_threads(nthreads)
                    {   
                        #pragma omp single nowait
                        {
                            solver(ua, &board, board.n_zeros, cutoff);
                        }
                    }
                    double time2 = omp_get_wtime() - time1;
                    avg_time += time2;
                }
                avg_time = avg_time / 5;
                printf("Board size: %d, Threads: %d, Cutoff: %d, Avg Time: %f\n", board_size, nthreads, cutoff, avg_time);
            }
        }
    }

    #else 

    int board_size = atoi(argv[1]);
    int nthreads = atoi(argv[2]);
    int cutoff = atoi(argv[3]);

    if(nthreads < 1) {
        printf("Invalid number of threads\n");
        return 1;
    }

    if(cutoff < 1 || cutoff > 500) {
        printf("Invalid cutoff\n");
        return 1;
    }

    board_t board;
    ua_t ua[MAX_CELLS];
    unsigned char board_array[MAX_SIDELENGTH][MAX_SIDELENGTH];
    int64_t rbits[MAX_SIDELENGTH];
    int64_t cbits[MAX_SIDELENGTH];
    int64_t bbits[MAX_SIDELENGTH];
    

    board.board = (unsigned char (*)[MAX_SIDELENGTH])board_array;
    board.rbits = rbits;
    board.cbits = cbits;
    board.bbits = bbits;

    if (!board_init(board_size, &board, ua)) {
        printf("Error initializing board\n");
        return 1;
    }
    
    double time = omp_get_wtime();
    print_board(&board, board.sidelength);
    #pragma omp parallel num_threads(nthreads) 
    {
        #pragma omp single nowait
        {
            solver(ua, &board, board.n_zeros, cutoff);
        }
    }
    
    time = omp_get_wtime() - time;

    printf("Board: %d Nthreads: %d recursion-cutoff: %d time taken: %f seconds \n", board_size, nthreads, cutoff, time);
    
#endif
    return 0;
}