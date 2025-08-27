#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include "verify.h"
#define MAX_SIDELENGTH 64

bool verify_row(unsigned char (*board)[MAX_SIDELENGTH], unsigned char row, unsigned char column, unsigned char value, unsigned char sidelength) {
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

bool verify_column(unsigned char (*board)[MAX_SIDELENGTH], unsigned char row, unsigned char column, unsigned char value, unsigned char sidelength) {
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

bool verify_square(unsigned char (*board)[MAX_SIDELENGTH], unsigned char row, unsigned char column, unsigned char value, unsigned char base) {
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

bool verify(board_t board_data) {
    unsigned char (*board)[MAX_SIDELENGTH] = board_data.board;
    unsigned char base = board_data.base;
    unsigned char sidelength = board_data.sidelength;
    for(int i = 0; i < sidelength; i++) {
        for(int j = 0; j < sidelength; j++) {
            if(board[i][j] == 0) {
                return false;
            }
            if(verify_row(board, i, j, board[i][j], sidelength) == false) {
                return false;
            }
            if(verify_column(board, i, j, board[i][j], sidelength) == false) {
                return false;
            }
            if(verify_square(board, i, j, board[i][j], base) == false) {
                return false;
            }
        }
    }
    return true;
}