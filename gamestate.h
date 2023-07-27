//
// Created by Ryan Baker on 6/13/23.
//

#ifndef CHESS_ENGINE_GAMESTATE_H
#define CHESS_ENGINE_GAMESTATE_H

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>

typedef uint64_t U64;

class GAMESTATE {
public:
    GAMESTATE(std::string startingPosition = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");
    ~GAMESTATE();

    void makeMove(class Move move);
    void undoMove();

    int mailbox[64]{};

    U64 w_pawn = 0, w_knight = 0, w_bishop = 0, w_rook = 0, w_queen = 0, w_king = 0,
        b_pawn = 0, b_knight = 0, b_bishop = 0, b_rook = 0, b_queen = 0, b_king = 0;
    U64* bitboards[12] = {
            &w_pawn, &w_knight, &w_bishop, &w_rook, &w_queen, &w_king,
            &b_pawn, &b_knight, &b_bishop, &b_rook, &b_queen, &b_king,
    };
    U64 pin_masks[64] = {0};
    U64 w_pieces = 0, b_pieces = 0, all_pieces = 0, empty_sqs = 0, pinned_pieces = 0;
    U64* armies[2] = {
            &b_pieces, &w_pieces
    };

    std::vector<Move> move_log;
    std::vector<Move> backup_move_log;

    bool whiteCanShortCastle = true;
    bool whiteCanLongCastle = true;
    bool blackCanShortCastle = true;
    bool blackCanLongCastle = true;

    int player_to_move = 1;

    const std::unordered_map<int, int> PIECE_NUM_TO_ARRAY_INDEX = {
            {1, 0}, // w_pawns
            {2, 1}, // w_knight
            {3, 2}, // w_bishop
            {4, 3}, // w_rook
            {5, 4}, // w_queen
            {6, 5}, // w_king
            {9, 6}, // b_pawns
            {10, 7}, // b_knight
            {11, 8}, // b_bishop
            {12, 9}, // b_rook
            {13, 10}, // b_queen
            {14, 11}, // b_king
    };

private:
    void loadFENString(std::string position);
    void loadBitboards();
};

#endif //CHESS_ENGINE_GAMESTATE_H
