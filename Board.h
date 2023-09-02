//
// Created by Ryan Baker on 8/21/23.
//

#ifndef CHESS_ENGINE_BOARD_H
#define CHESS_ENGINE_BOARD_H


#include "cstdlib"


typedef uint64_t bitboard;

struct Board {
public:
    bitboard w_pawn, w_knight, w_bishop, w_rook, w_queen, w_king;
    bitboard b_pawn, b_knight, b_bishop, b_rook, b_queen, b_king;
};


#endif //CHESS_ENGINE_BOARD_H
