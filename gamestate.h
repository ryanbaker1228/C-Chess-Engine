//
// Created by Ryan Baker on 6/13/23.
//

#ifndef CHESS_ENGINE_GAMESTATE_H
#define CHESS_ENGINE_GAMESTATE_H

#include "move.h"
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <stack>

typedef uint64_t U64;

namespace legalityBits {
    const int blackLongCastleShift = 0;
    const int blackShortCastleShift = 1;
    const int whiteLongCastleShift = 2;
    const int whiteShortCastleShift = 3;

    const int blackLongCastleMask = 0b0001;
    const int blackShortCastleMask = 0b0010;
    const int whiteLongCastleMask = 0b0100;
    const int whiteShortCastleMask = 0b1000;
    const int castleMask = 0b1111;
    const int whiteCanCastleMask = 0b1100;
    const int blackCanCastleMask = 0b0011;

    const int capturedPieceShift = 4;
    const int capturedPieceMask = 0b11110000;

    const int enPassantFileShift = 8;
    const int enPassantFileMask = 0b11100000000;

    const int enPassantLegalShift = 11;
    const int enPassantLegalMask = 0b100000000000;
}

enum Result {
    BlackWin, Draw, WhiteWin, Pending
};

class Gamestate {
private:
    Gamestate();
    void InitFENString(const std::string& position);
    void InitBitboards();
public:
    Gamestate(const Gamestate&) = delete;

    static Gamestate& Get() {
        static Gamestate instance;
        return instance;
    }

    void Seed(const std::string& position = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");

    void MakeMove(Move move);
    void UndoMove();

    std::array<int, 64> mailbox;
    U64 w_pawn, w_knight, w_bishop, w_rook, w_queen, w_king;
    U64 b_pawn, b_knight, b_bishop, b_rook, b_queen, b_king;

    U64* const bitboards[12] = {
            &w_pawn,
            &w_knight,
            &w_bishop,
            &w_rook,
            &w_queen,
            &w_king,
            &b_pawn,
            &b_knight,
            &b_bishop,
            &b_rook,
            &b_queen,
            &b_king,
    };

    U64 w_pieces = 0, b_pieces = 0, all_pieces = 0, empty_sqs = 0;

    int legality;

    std::stack<Move> moveLog;
    std::stack<int> legalityHistory;
    std::unordered_map<U64, int> threefoldHistory;

    Result result = Pending;

    bool whiteToMove;

    float gamePhase;

    U64 zobristKey;
};

const std::unordered_map<char, int> PieceChar2Number = {
        {'P', 1},
        {'N', 2},
        {'B', 3},
        {'R', 4},
        {'Q', 5},
        {'K', 6},
        {'p', 9},
        {'n', 10},
        {'b', 11},
        {'r', 12},
        {'q', 13},
        {'k', 14},
};

const std::unordered_map<char, int> CastlingChar2LegalityMask = {
        {'K', legalityBits::whiteShortCastleMask},
        {'Q', legalityBits::whiteLongCastleMask},
        {'k', legalityBits::blackShortCastleMask},
        {'q', legalityBits::blackLongCastleMask},
        {'-', 0},
};

const std::unordered_map<int, int> PieceNum2BitboardIndex = {
        {1, 0},
        {2, 1},
        {3, 2},
        {4, 3},
        {5, 4},
        {6, 5},
        {9, 6},
        {10, 7},
        {11, 8},
        {12, 9},
        {13, 10},
        {14, 11},
};

enum Piece {
    w_pawn = 1,
    w_knight = 2,
    w_bishop = 3,
    w_rook = 4,
    w_queen = 5,
    w_king = 6,
    b_pawn = 9,
    b_knight = 10,
    b_bishop = 11,
    b_rook = 12,
    b_queen = 13,
    b_king = 14,
};

enum PieceType {
    pawn,
    knight,
    bishop,
    rook,
    queen,
    king,
};

#endif //CHESS_ENGINE_GAMESTATE_H
