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

inline int isCapture(const Gamestate& gamestate, const int endSquare) {
    return MoveFlags::capture * (gamestate.mailbox[endSquare] > 0);
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

    std::vector<Move> legalMoves;

    void GenerateLegalMoves();
    int PerftTree(int depthPly);
    void PerftTest();

    void CalculateEnemyAttacks();
    void CalculateCheckMask();
    void CalculatePinMasks();

    void GenerateKingMoves();
    void GeneratePawnMoves();
    void GenerateKnightMoves();
    void GenerateBishopMoves();
    void GenerateRookMoves();
};

namespace MoveGenTest {
    const std::string positions[128] = {
            std::string("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"),
            std::string("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"),
            std::string("4k3/8/8/8/8/8/8/4K2R w K - 0 1"),
            std::string("4k3/8/8/8/8/8/8/R3K3 w Q - 0 1"),
            std::string("4k2r/8/8/8/8/8/8/4K3 w k - 0 1"),
            std::string("r3k3/8/8/8/8/8/8/4K3 w q - 0 1"),
            std::string("4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1"),
            std::string("r3k2r/8/8/8/8/8/8/4K3 w kq - 0 1"),
            std::string("8/8/8/8/8/8/6k1/4K2R w K - 0 1"),
            std::string("8/8/8/8/8/8/1k6/R3K3 w Q - 0 1"),
            std::string("4k2r/6K1/8/8/8/8/8/8 w k - 0 1"),
            std::string("r3k3/1K6/8/8/8/8/8/8 w q - 0 1"),
            std::string("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1"),
            std::string("r3k2r/8/8/8/8/8/8/1R2K2R w Kkq - 0 1"),
            std::string("r3k2r/8/8/8/8/8/8/2R1K2R w Kkq - 0 1"),
            std::string("r3k2r/8/8/8/8/8/8/R3K1R1 w Qkq - 0 1"),
            std::string("1r2k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1"),
            std::string("2r1k2r/8/8/8/8/8/8/R3K2R w KQk - 0 1"),
            std::string("r3k1r1/8/8/8/8/8/8/R3K2R w KQq - 0 1"),
            std::string("4k3/8/8/8/8/8/8/4K2R b K - 0 1"),
            std::string("4k3/8/8/8/8/8/8/R3K3 b Q - 0 1"),
            std::string("4k2r/8/8/8/8/8/8/4K3 b k - 0 1"),
            std::string("r3k3/8/8/8/8/8/8/4K3 b q - 0 1"),
            std::string("4k3/8/8/8/8/8/8/R3K2R b KQ - 0 1"),
            std::string("r3k2r/8/8/8/8/8/8/4K3 b kq - 0 1"),
            std::string("8/8/8/8/8/8/6k1/4K2R b K - 0 1"),
            std::string("8/8/8/8/8/8/1k6/R3K3 b Q - 0 1"),
            std::string("4k2r/6K1/8/8/8/8/8/8 b k - 0 1"),
            std::string("r3k3/1K6/8/8/8/8/8/8 b q - 0 1"),
            std::string("r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1"),
            std::string("r3k2r/8/8/8/8/8/8/1R2K2R b Kkq - 0 1"),
            std::string("r3k2r/8/8/8/8/8/8/2R1K2R b Kkq - 0 1"),
            std::string("r3k2r/8/8/8/8/8/8/R3K1R1 b Qkq - 0 1"),
            std::string("1r2k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1"),
            std::string("2r1k2r/8/8/8/8/8/8/R3K2R b KQk - 0 1"),
            std::string("r3k1r1/8/8/8/8/8/8/R3K2R b KQq - 0 1"),
            std::string("8/1n4N1/2k5/8/8/5K2/1N4n1/8 w - - 0 1"),
            std::string("8/1k6/8/5N2/8/4n3/8/2K5 w - - 0 1"),
            std::string("8/8/4k3/3Nn3/3nN3/4K3/8/8 w - - 0 1"),
            std::string("K7/8/2n5/1n6/8/8/8/k6N w - - 0 1"),
            std::string("k7/8/2N5/1N6/8/8/8/K6n w - - 0 1"),
            std::string("8/1n4N1/2k5/8/8/5K2/1N4n1/8 b - - 0 1"),
            std::string("8/1k6/8/5N2/8/4n3/8/2K5 b - - 0 1"),
            std::string("8/8/3K4/3Nn3/3nN3/4k3/8/8 b - - 0 1"),
            std::string("K7/8/2n5/1n6/8/8/8/k6N b - - 0 1"),
            std::string("k7/8/2N5/1N6/8/8/8/K6n b - - 0 1"),
            std::string("B6b/8/8/8/2K5/4k3/8/b6B w - - 0 1"),
            std::string("8/8/1B6/7b/7k/8/2B1b3/7K w - - 0 1"),
            std::string("k7/B7/1B6/1B6/8/8/8/K6b w - - 0 1"),
            std::string("K7/b7/1b6/1b6/8/8/8/k6B w - - 0 1"),
            std::string("B6b/8/8/8/2K5/5k2/8/b6B b - - 0 1"),
            std::string("8/8/1B6/7b/7k/8/2B1b3/7K b - - 0 1"),
            std::string("k7/B7/1B6/1B6/8/8/8/K6b b - - 0 1"),
            std::string("K7/b7/1b6/1b6/8/8/8/k6B b - - 0 1"),
            std::string("7k/RR6/8/8/8/8/rr6/7K w - - 0 1"),
            std::string("R6r/8/8/2K5/5k2/8/8/r6R w - - 0 1"),
            std::string("7k/RR6/8/8/8/8/rr6/7K b - - 0 1"),
            std::string("R6r/8/8/2K5/5k2/8/8/r6R b - - 0 1"),
            std::string("6kq/8/8/8/8/8/8/7K w - - 0 1"),
            std::string("6KQ/8/8/8/8/8/8/7k b - - 0 1"),
            std::string("K7/8/8/3Q4/4q3/8/8/7k w - - 0 1"),
            std::string("6qk/8/8/8/8/8/8/7K b - - 0 1"),
            std::string("6KQ/8/8/8/8/8/8/7k b - - 0 1"),
            std::string("K7/8/8/3Q4/4q3/8/8/7k b - - 0 1"),
            std::string("8/8/8/8/8/K7/P7/k7 w - - 0 1"),
            std::string("8/8/8/8/8/7K/7P/7k w - - 0 1"),
            std::string("K7/p7/k7/8/8/8/8/8 w - - 0 1"),
            std::string("7K/7p/7k/8/8/8/8/8 w - - 0 1"),
            std::string("8/2k1p3/3pP3/3P2K1/8/8/8/8 w - - 0 1"),
            std::string("8/8/8/8/8/K7/P7/k7 b - - 0 1"),
            std::string("8/8/8/8/8/7K/7P/7k b - - 0 1"),
            std::string("K7/p7/k7/8/8/8/8/8 b - - 0 1"),
            std::string("7K/7p/7k/8/8/8/8/8 b - - 0 1"),
            std::string("8/2k1p3/3pP3/3P2K1/8/8/8/8 b - - 0 1"),
            std::string("8/8/8/8/8/4k3/4P3/4K3 w - - 0 1"),
            std::string("4k3/4p3/4K3/8/8/8/8/8 b - - 0 1"),
            std::string("8/8/7k/7p/7P/7K/8/8 w - - 0 1"),
            std::string("8/8/k7/p7/P7/K7/8/8 w - - 0 1"),
            std::string("8/8/3k4/3p4/3P4/3K4/8/8 w - - 0 1"),
            std::string("8/3k4/3p4/8/3P4/3K4/8/8 w - - 0 1"),
            std::string("8/8/3k4/3p4/8/3P4/3K4/8 w - - 0 1"),
            std::string("k7/8/3p4/8/3P4/8/8/7K w - - 0 1"),
            std::string("8/8/7k/7p/7P/7K/8/8 b - - 0 1"),
            std::string("8/8/k7/p7/P7/K7/8/8 b - - 0 1"),
            std::string("8/8/3k4/3p4/3P4/3K4/8/8 b - - 0 1"),
            std::string("8/3k4/3p4/8/3P4/3K4/8/8 b - - 0 1"),
            std::string("8/8/3k4/3p4/8/3P4/3K4/8 b - - 0 1"),
            std::string("k7/8/3p4/8/3P4/8/8/7K b - - 0 1"),
            std::string("7k/3p4/8/8/3P4/8/8/K7 w - - 0 1"),
            std::string("7k/8/8/3p4/8/8/3P4/K7 w - - 0 1"),
            std::string("k7/8/8/7p/6P1/8/8/K7 w - - 0 1"),
            std::string("k7/8/7p/8/8/6P1/8/K7 w - - 0 1"),
            std::string("k7/8/8/6p1/7P/8/8/K7 w - - 0 1"),
            std::string("k7/8/6p1/8/8/7P/8/K7 w - - 0 1"),
            std::string("k7/8/8/3p4/4p3/8/8/7K w - - 0 1"),
            std::string("k7/8/3p4/8/8/4P3/8/7K w - - 0 1"),
            std::string("7k/3p4/8/8/3P4/8/8/K7 b - - 0 1"),
            std::string("7k/8/8/3p4/8/8/3P4/K7 b - - 0 1"),
            std::string("k7/8/8/7p/6P1/8/8/K7 b - - 0 1"),
            std::string("k7/8/7p/8/8/6P1/8/K7 b - - 0 1"),
            std::string("k7/8/8/6p1/7P/8/8/K7 b - - 0 1"),
            std::string("k7/8/6p1/8/8/7P/8/K7 b - - 0 1"),
            std::string("k7/8/8/3p4/4p3/8/8/7K b - - 0 1"),
            std::string("k7/8/3p4/8/8/4P3/8/7K b - - 0 1"),
            std::string("7k/8/8/p7/1P6/8/8/7K w - - 0 1"),
            std::string("7k/8/p7/8/8/1P6/8/7K w - - 0 1"),
            std::string("7k/8/8/1p6/P7/8/8/7K w - - 0 1"),
            std::string("7k/8/1p6/8/8/P7/8/7K w - - 0 1"),
            std::string("k7/7p/8/8/8/8/6P1/K7 w - - 0 1"),
            std::string("k7/6p1/8/8/8/8/7P/K7 w - - 0 1"),
            std::string("3k4/3pp3/8/8/8/8/3PP3/3K4 w - - 0 1"),
            std::string("7k/8/8/p7/1P6/8/8/7K b - - 0 1"),
            std::string("7k/8/p7/8/8/1P6/8/7K b - - 0 1"),
            std::string("7k/8/8/1p6/P7/8/8/7K b - - 0 1"),
            std::string("7k/8/1p6/8/8/P7/8/7K b - - 0 1"),
            std::string("k7/7p/8/8/8/8/6P1/K7 b - - 0 1"),
            std::string("k7/6p1/8/8/8/8/7P/K7 b - - 0 1"),
            std::string("3k4/3pp3/8/8/8/8/3PP3/3K4 b - - 0 1"),
            std::string("8/Pk6/8/8/8/8/6Kp/8 w - - 0 1"),
            std::string("n1n5/1Pk5/8/8/8/8/5Kp1/5N1N w - - 0 1"),
            std::string("8/PPPk4/8/8/8/8/4Kppp/8 w - - 0 1"),
            std::string("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N w - - 0 1"),
            std::string("8/Pk6/8/8/8/8/6Kp/8 b - - 0 1"),
            std::string("n1n5/1Pk5/8/8/8/8/5Kp1/5N1N b - - 0 1"),
            std::string("8/PPPk4/8/8/8/8/4Kppp/8 b - - 0 1"),
            std::string("n1n5/PPPk4/8/8/8/8/4Kppp/5N1N b - - 0 1"),
            std::string("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1"),
            std::string("rnbqkb1r/ppppp1pp/7n/4Pp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3")
    };

    const long perftResults[128][6] = {
            {20, 400, 8902, 197281, 4865609, 119060324},
            {48, 2039, 97862, 4085603, 193690690, 8031647685},
            {15, 66, 1197, 7059, 133987, 764643},
            {16, 71, 1287, 7626, 145232, 846648},
            {5, 75, 459, 8290, 47635, 899442},
            {5, 80, 493, 8897, 52710, 1001523},
            {26, 112, 3189, 17945, 532933, 2788982},
            {5, 130, 782, 22180, 118882, 3517770},
            {12, 38, 564, 2219, 37735, 185867},
            {15, 65, 1018, 4573, 80619, 413018},
            {3, 32, 134, 2073, 10485, 179869},
            {4, 49, 243, 3991, 20780, 367724},
            {26, 568, 13744, 314346, 7594526, 179862938},
            {25, 567, 14095, 328965, 8153719, 195629489},
            {25, 548, 13502, 312835, 7736373, 184411439},
            {25, 547, 13579, 316214, 7878456, 189224276},
            {26, 583, 14252, 334705, 8198901, 198328929},
            {25, 560, 13592, 317324, 7710115, 185959088},
            {25, 560, 13607, 320792, 7848606, 190755813},
            {5, 75, 459, 8290, 47635, 899442},
            {5, 80, 493, 8897, 52710, 1001523},
            {15, 66, 1197, 7059, 133987, 764643},
            {16, 71, 1287, 7626, 145232, 846648},
            {45, 130, 782, 22180, 118882, 3517770},
            {26, 112, 3189, 17945, 532933, 2788982},
            {3, 32, 134, 2073, 10485, 179869},
            {4, 49, 243, 3991, 20780, 367724},
            {12, 38, 564, 2219, 37735, 185867},
            {15, 65, 1018, 4573, 80619, 413018},
            {26, 568, 13744, 314346, 7594526, 179862938},
            {26, 583, 14252, 334705, 8198901, 198328929},
            {25, 560, 13592, 317324, 7710115, 185959088},
            {25, 560, 13607, 320792, 7848606, 190755813},
            {25, 567, 14095, 328965, 8153719, 195629489},
            {25, 548, 13502, 312835, 7736373, 184411439},
            {25, 547, 13579, 316214, 7878456, 189224276},
            {14, 195, 2760, 38675, 570726, 8107539},
            {11, 156, 1636, 20534, 223507, 2594412},
            {19, 289, 4442, 73584, 1198299, 19870403},
            {3, 51, 345, 5301, 38348, 588695},
            {17, 54, 835, 5910, 92250, 688780},
            {15, 193, 2816, 40039, 582642, 8503277},
            {16, 180, 2290, 24640, 288141, 3147566},
            {4, 68, 1118, 16199, 281190, 4405103},
            {17, 54, 835, 5910, 92250, 688780},
            {3, 51, 345, 5301, 38348, 588695},
            {17, 278, 4607, 76778, 1320507, 22823890},
            {21, 316, 5744, 93338, 1713368, 28861171},
            {21, 144, 3242, 32955, 787524, 7881673},
            {7, 143, 1416, 31787, 310862, 7382896},
            {6, 106, 1829, 31151, 530585, 9250746},
            {17, 309, 5133, 93603, 1591064, 29027891},
            {7, 143, 1416, 31787, 310862, 7382896},
            {21, 144, 3242, 32955, 787524, 7881673},
            {19, 275, 5300, 104342, 2161211, 44956585},
            {36, 1027, 29215, 771461, 20506480, 525169084},
            {19, 275, 5300, 104342, 2161211, 44956585},
            {36, 1027, 29227, 771368, 20521342, 524966748},
            {2, 36, 143, 3637, 14893, 391507},
            {2, 36, 143, 3637, 14893, 391507},
            {6, 35, 495, 8349, 166741, 3370175},
            {22, 43, 1015, 4167, 105749, 419369},
            {2, 36, 143, 3637, 14893, 391507},
            {6, 35, 495, 8349, 166741, 3370175},
            {3, 7, 43, 199, 1347, 6249},
            {3, 7, 43, 199, 1347, 6249},
            {1, 3, 12, 80, 342, 2343},
            {1, 3, 12, 80, 342, 2343},
            {7, 35, 210, 1091, 7028, 34834},
            {1, 3, 12, 80, 342, 2343},
            {1, 3, 12, 80, 342, 2343},
            {3, 7, 43, 199, 1347, 6249},
            {3, 7, 43, 199, 1347, 6249},
            {5, 35, 182, 1091, 5408, 34822},
            {2, 8, 44, 282, 1814, 11848},
            {2, 8, 44, 282, 1814, 11848},
            {3, 9, 57, 360, 1969, 10724},
            {3, 9, 57, 360, 1969, 10724},
            {5, 25, 180, 1294, 8296, 53138},
            {8, 61, 483, 3213, 23599, 157093},
            {8, 61, 411, 3213, 21637, 158065},
            {4, 15, 90, 534, 3450, 20960},
            {3, 9, 57, 360, 1969, 10724},
            {3, 9, 57, 360, 1969, 10724},
            {5, 25, 180, 1294, 8296, 53138},
            {8, 61, 411, 3213, 21637, 158065},
            {8, 61, 483, 3213, 23599, 157093},
            {4, 15, 89, 537, 3309, 21104},
            {4, 19, 117, 720, 4661, 32191},
            {5, 19, 116, 716, 4786, 30980},
            {5, 22, 139, 877, 6112, 41874},
            {4, 16, 101, 637, 4354, 29679},
            {5, 22, 139, 877, 6112, 41874},
            {4, 16, 101, 637, 4354, 29679},
            {3, 15, 84, 573, 3013, 22886},
            {4, 16, 101, 637, 4271, 28662},
            {5, 19, 117, 720, 5014, 32167},
            {4, 19, 117, 712, 4658, 30749},
            {5, 22, 139, 877, 6112, 41874},
            {4, 16, 101, 637, 4354, 29679},
            {5, 22, 139, 877, 6112, 41874},
            {4, 16, 101, 637, 4354, 29679},
            {5, 15, 102, 569, 4337, 22579},
            {4, 16, 101, 637, 4271, 28662},
            {5, 22, 139, 877, 6112, 41874},
            {4, 16, 101, 637, 4354, 29679},
            {5, 22, 139, 877, 6112, 41874},
            {4, 16, 101, 637, 4354, 29679},
            {5, 25, 161, 1035, 7574, 55338},
            {5, 25, 161, 1035, 7574, 55338},
            {7, 49, 378, 2902, 24122, 199002},
            {5, 22, 139, 877, 6112, 41874},
            {4, 16, 101, 637, 4354, 29679},
            {5, 22, 139, 877, 6112, 41874},
            {4, 16, 101, 637, 4354, 29679},
            {5, 25, 161, 1035, 7574, 55338},
            {5, 25, 161, 1035, 7574, 55338},
            {7, 49, 378, 2902, 24122, 199002},
            {11, 97, 887, 8048, 90606, 1030499},
            {24, 421, 7421, 124608, 2193768, 37665329},
            {18, 270, 4699, 79355, 1533145, 28859283},
            {24, 496, 9483, 182838, 3605103, 71179139},
            {11, 97, 887, 8048, 90606, 1030499},
            {24, 421, 7421, 124608, 2193768, 37665329},
            {18, 270, 4699, 79355, 1533145, 28859283},
            {24, 496, 9483, 182838, 3605103, 71179139},
            {14, 191, 2812, 43238, 674624, 11030083},
            {31, 570, 17546, 351806, 1113976, 244063299}
    };
}

namespace MovementTables {
    void LoadTables();

    inline U64 knightMoves[64];
    inline U64 bishopMoves[64][7];
    inline U64 rookMoves[64][7];
    inline U64 kingMoves[64];
}
#endif //CHESS_ENGINE_MOVEGEN_H