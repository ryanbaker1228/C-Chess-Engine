//
// Created by Ryan Baker on 7/1/23.
//

#ifndef CHESS_ENGINE_MOVEGEN_H
#define CHESS_ENGINE_MOVEGEN_H

#include "gamestate.h"
#include "move.h"
#include <iostream>
#include <array>
#include <climits>
#include <immintrin.h>


#define squareOf(bitboard) _tzcnt_u64(bitboard)

inline int getLSB(const U64 number) {
    return static_cast<int>(log2(number & -number));
}

inline U64 getMSB(const U64 number) {
    return (1ULL << 63) >> (63 - static_cast<int>(floor(log2(number | 1))));
}

inline int popLSB(U64& number) {
    int least_significant_bit = static_cast<int>(log2(number & -number));
    number &= number - 1;
    return least_significant_bit;
}

inline int isCapture(const Gamestate& gamestate, const int endSquare) {
    return MoveFlags::capture * (gamestate.mailbox[endSquare] > 0);
}

inline U64 diagonalSliders(const Gamestate& gamestate, bool isWhite) {
    if (isWhite) return gamestate.w_bishop | gamestate.w_queen;
    return gamestate.b_bishop | gamestate.b_queen;
}

namespace Board {
    namespace Squares {
        [[maybe_unused]] const int a1 = 0;
        [[maybe_unused]] const int b1 = 1;
        [[maybe_unused]] const int c1 = 2;
        [[maybe_unused]] const int d1 = 3;
        [[maybe_unused]] const int e1 = 4;
        [[maybe_unused]] const int f1 = 5;
        [[maybe_unused]] const int g1 = 6;
        [[maybe_unused]] const int h1 = 7;
        [[maybe_unused]] const int a2 = 8;
        [[maybe_unused]] const int b2 = 9;
        [[maybe_unused]] const int c2 = 10;
        [[maybe_unused]] const int d2 = 11;
        [[maybe_unused]] const int e2 = 12;
        [[maybe_unused]] const int f2 = 13;
        [[maybe_unused]] const int g2 = 14;
        [[maybe_unused]] const int h2 = 15;
        [[maybe_unused]] const int a3 = 16;
        [[maybe_unused]] const int b3 = 17;
        [[maybe_unused]] const int c3 = 18;
        [[maybe_unused]] const int d3 = 19;
        [[maybe_unused]] const int e3 = 20;
        [[maybe_unused]] const int f3 = 21;
        [[maybe_unused]] const int g3 = 22;
        [[maybe_unused]] const int h3 = 23;
        [[maybe_unused]] const int a4 = 24;
        [[maybe_unused]] const int b4 = 25;
        [[maybe_unused]] const int c4 = 26;
        [[maybe_unused]] const int d4 = 27;
        [[maybe_unused]] const int e4 = 28;
        [[maybe_unused]] const int f4 = 29;
        [[maybe_unused]] const int g4 = 30;
        [[maybe_unused]] const int h4 = 31;
        [[maybe_unused]] const int a5 = 32;
        [[maybe_unused]] const int b5 = 33;
        [[maybe_unused]] const int c5 = 34;
        [[maybe_unused]] const int d5 = 35;
        [[maybe_unused]] const int e5 = 36;
        [[maybe_unused]] const int f5 = 37;
        [[maybe_unused]] const int g5 = 38;
        [[maybe_unused]] const int h5 = 39;
        [[maybe_unused]] const int a6 = 40;
        [[maybe_unused]] const int b6 = 41;
        [[maybe_unused]] const int c6 = 42;
        [[maybe_unused]] const int d6 = 43;
        [[maybe_unused]] const int e6 = 44;
        [[maybe_unused]] const int f6 = 45;
        [[maybe_unused]] const int g6 = 46;
        [[maybe_unused]] const int h6 = 47;
        [[maybe_unused]] const int a7 = 48;
        [[maybe_unused]] const int b7 = 49;
        [[maybe_unused]] const int c7 = 50;
        [[maybe_unused]] const int d7 = 51;
        [[maybe_unused]] const int e7 = 52;
        [[maybe_unused]] const int f7 = 53;
        [[maybe_unused]] const int g7 = 54;
        [[maybe_unused]] const int h7 = 55;
        [[maybe_unused]] const int a8 = 56;
        [[maybe_unused]] const int b8 = 57;
        [[maybe_unused]] const int c8 = 58;
        [[maybe_unused]] const int d8 = 59;
        [[maybe_unused]] const int e8 = 60;
        [[maybe_unused]] const int f8 = 61;
        [[maybe_unused]] const int g8 = 62;
        [[maybe_unused]] const int h8 = 63;
    }
    namespace Files {
        [[maybe_unused]] const U64 aFile = 0x0101010101010101;
        [[maybe_unused]] const U64 bFile = aFile << 1;
        [[maybe_unused]] const U64 cFile = aFile << 2;
        [[maybe_unused]] const U64 dFile = aFile << 3;
        [[maybe_unused]] const U64 eFile = aFile << 4;
        [[maybe_unused]] const U64 fFile = aFile << 5;
        [[maybe_unused]] const U64 gFile = aFile << 6;
        [[maybe_unused]] const U64 hFile = aFile << 7;
    }
    namespace Ranks {
        [[maybe_unused]] const U64 rank_1 = 0xff;
        [[maybe_unused]] const U64 rank_2 = rank_1 << 8;
        [[maybe_unused]] const U64 rank_3 = rank_1 << 16;
        [[maybe_unused]] const U64 rank_4 = rank_1 << 24;
        [[maybe_unused]] const U64 rank_5 = rank_1 << 32;
        [[maybe_unused]] const U64 rank_6 = rank_1 << 40;
        [[maybe_unused]] const U64 rank_7 = rank_1 << 48;
        [[maybe_unused]] const U64 rank_8 = rank_1 << 56;
    }
}

namespace PawnMoves {
    inline U64 oneStep(const bool whiteToMove, const U64 bitboard) {
        return whiteToMove ? bitboard << 8 : bitboard >> 8;
    }

    inline U64 twoStep(const bool whiteToMove, const U64 bitboard) {
        return whiteToMove ? bitboard << 16 : bitboard >> 16;
    }

    inline U64 leftwardCapt(const bool whiteToMove, const U64 bitboard) {
        return whiteToMove ?
               (bitboard & ~Board::Files::aFile) << 7 :
               (bitboard & ~Board::Files::hFile) >> 7;
    }

    inline U64 rightwardCapt(const bool whiteToMove, const U64 bitboard) {
        return whiteToMove ?
               (bitboard & ~Board::Files::hFile) << 9 :
               (bitboard & ~Board::Files::aFile) >> 9;
    }

    inline U64 allCaptures(const bool whiteToMove, const U64 bitboard) {
        return whiteToMove ?
               (bitboard & ~Board::Files::aFile) << 7 | (bitboard & ~Board::Files::hFile) << 9 :
               (bitboard & ~Board::Files::hFile) >> 7 | (bitboard & ~Board::Files::aFile) >> 9;
    }
}

inline U64 EnemyPawns() {
    return Gamestate::Get().whiteToMove ? Gamestate::Get().b_pawn : Gamestate::Get().w_pawn;
}

inline U64 FriendlyPawns() {
    return Gamestate::Get().whiteToMove ? Gamestate::Get().w_pawn : Gamestate::Get().b_pawn;
}

inline U64 EnemyKnights() {
    return Gamestate::Get().whiteToMove ? Gamestate::Get().b_knight : Gamestate::Get().w_knight;
}

inline U64 FriendlyKnights() {
    return Gamestate::Get().whiteToMove ? Gamestate::Get().w_knight : Gamestate::Get().b_knight;
}

inline U64 EnemyBishops() {
    return Gamestate::Get().whiteToMove ? Gamestate::Get().b_bishop : Gamestate::Get().w_bishop;
}

inline U64 FriendlyBishops() {
    return Gamestate::Get().whiteToMove ? Gamestate::Get().w_bishop : Gamestate::Get().b_bishop;
}

inline U64 EnemyRooks() {
    return Gamestate::Get().whiteToMove ? Gamestate::Get().b_rook : Gamestate::Get().w_rook;
}

inline U64 FriendlyRooks() {
    return Gamestate::Get().whiteToMove ? Gamestate::Get().w_rook : Gamestate::Get().b_rook;
}

inline U64 EnemyQueen() {
    return Gamestate::Get().whiteToMove ? Gamestate::Get().b_queen : Gamestate::Get().w_queen;
}

inline U64 FriendlyQueen() {
    return Gamestate::Get().whiteToMove ? Gamestate::Get().w_queen : Gamestate::Get().b_queen;
}

inline U64 EnemyKing() {
    return Gamestate::Get().whiteToMove ? Gamestate::Get().b_king : Gamestate::Get().w_king;
}

inline U64 FriendlyKing() {
    return Gamestate::Get().whiteToMove ? Gamestate::Get().w_king : Gamestate::Get().b_king;
}

inline U64 EnemyPieces() {
    return Gamestate::Get().whiteToMove ? Gamestate::Get().b_pieces : Gamestate::Get().w_pieces;
}

inline U64 FriendlyPieces() {
    return Gamestate::Get().whiteToMove ? Gamestate::Get().w_pieces : Gamestate::Get().b_pieces;
}

inline bool isQueen(int square) {
    return (Gamestate::Get().mailbox[square] & 0b0111) == 0b0101;
}

inline U64 MinorPieces() {
    Gamestate& gamestate = Gamestate::Get();
    return gamestate.w_knight | gamestate.b_knight | gamestate.w_bishop | gamestate.b_bishop;
}

inline U64 MajorPieces() {
    Gamestate& gamestate = Gamestate::Get();
    return gamestate.w_rook | gamestate.b_rook | gamestate.w_queen | gamestate.b_queen;
}

namespace MovementTables {
    void LoadTables();

    inline U64 knightMoves[64];
    inline U64 bishopMoves[64][7];
    inline U64 rookMoves[64][7];
    inline U64 kingMoves[64];
}

class MoveGenerator {
private:
    MoveGenerator();
public:
    static MoveGenerator& Get() {
        static MoveGenerator instance;
        return instance;
    }

    MoveGenerator(const MoveGenerator&) = delete;

    U64 pinnedPieces, checkMask, enemyAttacks;
    std::array<U64, 64> pinMasks;
    bool king_is_in_double_check;
    bool king_is_in_check;

    std::vector<Move> legalMoves;

    void GenerateLegalMoves();
    int PerftTree(int depthPly);

    void CalculateEnemyAttacks();
    void CalculateCheckMask();
    void CalculatePinMasks();

    void GenerateKingMoves();
    void GeneratePawnMoves();
    void GenerateKnightMoves();
    void GenerateBishopMoves();
    void GenerateRookMoves();
};

#endif //CHESS_ENGINE_MOVEGEN_H