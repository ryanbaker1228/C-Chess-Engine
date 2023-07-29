//
// Created by Ryan Baker on 7/1/23.
//

#ifndef CHESS_ENGINE_MOVEGEN_H
#define CHESS_ENGINE_MOVEGEN_H

#include "gamestate.h"
#include "move.h"
#include <iostream>
#include <climits>


namespace MoveGenerator {
    std::vector<class Move> GenerateLegalMoves(GAMESTATE& gamestate);

    U64 CreateCheckMask(const GAMESTATE& gamestate, U64 kingPos);
    U64 FindAttackedSquares(const GAMESTATE& gamestate, U64 friendlyPieces, U64 kingPos);
    void FindPinnedPieces(GAMESTATE& gamestate);

    void GeneratePawnMoves(const GAMESTATE& gamestate, std::vector<class Move>& legalMoves, U64 checkMask);
    void GenerateKnightMoves(const GAMESTATE& gamestate, std::vector<class Move>& legalMoves, U64 checkMask);
    void GenerateDiagonalSliderMoves(const GAMESTATE& gamestate, std::vector<class Move>& legalMoves, U64 checkMask);
    void GenerateOrthogonalSliderMoves(const GAMESTATE& gamestate, std::vector<class Move>& legalMoves, U64 checkMask);
    void GenerateKingMoves(const GAMESTATE& gamestate, std::vector<class Move>& legalMoves, U64 enemyAttacks);

    inline int isCapture(const GAMESTATE& gamestate, const int endSquare) {
        return MoveFlags::capture * (gamestate.mailbox[endSquare] > 0);
    }

    void TestMoveGenerator();
}

const int WHITE_WIN = 999999;
const int BLACK_WIN = -999999;

int createMoveTree(const GAMESTATE& gamestate, int depth_ply);
int MoveSearch(GAMESTATE* gamestate, int depth_ply, int alpha, int beta);

namespace MovementTables {
    void LoadTables();

    inline U64 knightMoves[64];
    inline U64 bishopMoves[64][7];
    inline U64 rookMoves[64][7];
    inline U64 kingMoves[64];
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

#endif //CHESS_ENGINE_MOVEGEN_H