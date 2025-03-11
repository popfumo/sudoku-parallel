#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <omp.h>
volatile bool global_flag = false;

typedef struct {
    unsigned char base;
    unsigned char sidelength;
    short int n_zeros;
    unsigned char N;
    unsigned char **board;
} board_t;

// FIXME: Maybe SoA instead of AoS? 
typedef struct {
    unsigned char x; // row
    unsigned char y; // column
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


ua_t *board_init(board_t *board) {
    FILE *file = fopen("boards/board_36x36.dat", "r");
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
    
    fclose(file); 
    return ua;
   
    

}

bool check_row(unsigned char **board, unsigned char row, unsigned char column, unsigned char value, unsigned char sidelength) {

    for(int i = 0; i < sidelength; i++) {
        if(i == column) {
            continue;
        }
        if(board[row][i] == value) {
            return false;
        }
    }
    return true;
}
bool check_column(unsigned char **board, unsigned char row, unsigned char column, unsigned char value, unsigned char sidelength) {
    for(int i = 0; i < sidelength; i++) {
        if(i == row) {
            continue;
        }
        if(board[i][column] == value) {
            return false;
        }
    }
    return true;
}
bool check_square(unsigned char **board, unsigned char row, unsigned char column, unsigned char value, unsigned char base) {
    unsigned char start_row = row - row % base;
    unsigned char start_column = column - column % base;
    for(int i = 0; i < base; i++) {
        for(int j = 0; j < base; j++) {
            if (i + start_row == row && j + start_column == column) {
                continue;
            }
            if(board[i + start_row][j + start_column] == value) {
                return false;
            }
        }
    }
    return true;
}

bool validate(unsigned char **board, unsigned char row, unsigned char column, unsigned char value, unsigned char base, unsigned char sidelength) {
    if(check_row(board, row, column, value, sidelength) == false) {
        return false;
    }
    if(check_column(board, row, column, value, sidelength) == false) {
        return false;
    }
    if(check_square(board, row, column, value, base) == false) {
        return false;
    }
    return true;
}

bool solver(ua_t *ua, board_t *board, short int zeroes) {
    if(zeroes == 0) {
        return true;
    }
    ua_t index = ua[zeroes-1];
    for(int i = 1; i <= board->sidelength; i++) {
        board->board[index.x][index.y] = i;
        if(validate(board->board, index.x, index.y, i, board->base, board->sidelength)) {
            if(solver(ua, board, zeroes - 1)) {
                return true;
            }
        }
    }
    board->board[index.x][index.y] = 0;
    return false;
}

int main(int argc, char *argv[]) {
    board_t *board = malloc(sizeof(board_t));
    ua_t *ua = board_init(board);
    if (board == NULL) {
        printf("Error initializing board\n");
        return 1;
    }
    printf("no zeros: %d\n", board->n_zeros);
    //print_ua(ua, board->n_zeros);
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
    free(ua);
    for(int i = 0; i < board->sidelength; i++) {
        free(board->board[i]);
    }
    free(board->board);

    return 0;
}