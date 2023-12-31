//
// Created by Ryan Baker on 7/11/23.
//

#include "evaluation.h"
#include "bitUtils.h"
#include "movegen.h"
#include <cmath>


int Evaluator::StaticEvaluation() {
    ++callCount;
    Gamestate& gamestate = Gamestate::Get();

    if (gamestate.result == Draw) return 0;

    int perspective = gamestate.whiteToMove ? 1 : -1;

    /* Count Material */
    CountMaterial();

    int eval = material + EvaluatePcSqTables() + MopUpEvaluation() + EvaluateMobility() + EvaluateStructure();
    return perspective * eval;
}

void Evaluator::CountMaterial() {
    Gamestate& gamestate = Gamestate::Get();
    material = 0;

    int current_pawn_val = (1- gamestate.gamePhase) * PieceValues::midGamePawn + gamestate.gamePhase * PieceValues::endGamePawn;
    int current_knight_val = (1- gamestate.gamePhase) * PieceValues::midGameKnight + gamestate.gamePhase * PieceValues::endGameKnight;
    int current_bishop_val = (1- gamestate.gamePhase) * PieceValues::midGameBishop + gamestate.gamePhase * PieceValues::endGameBishop;
    int current_rook_val = (1- gamestate.gamePhase) * PieceValues::midGameRook + gamestate.gamePhase * PieceValues::endGameRook;
    int current_queen_val = (1- gamestate.gamePhase) * PieceValues::midGameQueen + gamestate.gamePhase * PieceValues::endGameQueen;

    material += current_pawn_val * (bit_cnt(gamestate.w_pawn) - bit_cnt(gamestate.b_pawn));
    material += current_knight_val * (bit_cnt(gamestate.w_knight) - bit_cnt(gamestate.b_knight));
    material += current_bishop_val * (bit_cnt(gamestate.w_bishop) - bit_cnt(gamestate.b_bishop));
    material += current_rook_val * (bit_cnt(gamestate.w_rook) - bit_cnt(gamestate.b_rook));
    material += current_queen_val * (bit_cnt(gamestate.w_queen) - bit_cnt(gamestate.b_queen));
}

int Evaluator::EvaluatePcSqTables() {
    Gamestate& gamestate = Gamestate::Get();
    int value = 0;
    U64 bitboard;

    for (int piece = 0; piece < 12; ++piece) {
        bitboard = *gamestate.bitboards[piece];
        while (bitboard) {
            int square = popLSB(bitboard);
            value += (1 - gamestate.gamePhase) * PcSqTables::midGameTables[piece][square] +
                           gamestate.gamePhase * PcSqTables::endGameTables[piece][square];
        }
    }
    return value;
}

int Evaluator::MopUpEvaluation() {
    int value = 0;
    Gamestate& gamestate = Gamestate::Get();

    if (Gamestate::Get().gamePhase > 0.75 && material >= 500) {
        value += 10 * PcSqTables::centerManhattanDistance[squareOf(gamestate.b_king)];
        value -= 4 * ManhattanDistance(squareOf(gamestate.w_king), squareOf(gamestate.b_king));
        value -= 10 * PcSqTables::centerManhattanDistance[squareOf(gamestate.w_king)];
    } else if (Gamestate::Get().gamePhase > 0.75 && material <= -500) {
        value -= 10 * PcSqTables::centerManhattanDistance[squareOf(gamestate.w_king)];
        value += 4 * ManhattanDistance(squareOf(gamestate.w_king), squareOf(gamestate.b_king));
        value += 10 * PcSqTables::centerManhattanDistance[squareOf(gamestate.b_king)];
    }

    if (bit_cnt(Gamestate::Get().all_pieces) == 4 &&
        bit_cnt(Gamestate::Get().w_bishop | Gamestate::Get().b_bishop) == 1 &&
        bit_cnt(Gamestate::Get().w_knight | Gamestate::Get().b_knight) == 1) {
        int colorIndex = Gamestate::Get().mailbox[squareOf(Gamestate::Get().w_bishop | Gamestate::Get().b_bishop)] % 2;

        if (colorIndex == 0) {
            value -= 20 * std::min(ManhattanDistance(Board::Squares::a8, EnemyKing()),
                                   ManhattanDistance(Board::Squares::h1, EnemyKing()));
        } else {
            value -= 20 * std::min(ManhattanDistance(Board::Squares::a1, EnemyKing()),
                                   ManhattanDistance(Board::Squares::h8, EnemyKing()));
        }
    }
    return Gamestate::Get().whiteToMove ? value : -value;
}

int Evaluator::EvaluateMobility() {
    Gamestate& gamestate = Gamestate::Get();
    int value = 50 * log2(MoveGenerator::Get().CountLegalMoves());

    // Placing a piece in front of a central pawn before it has moves is bad.
    value += 50 * bit_cnt(~gamestate.empty_sqs & (1ULL << Board::Squares::d6 | 1ULL << Board::Squares::e6) & gamestate.b_pawn >> 8);
    value -= 50 * bit_cnt(gamestate.all_pieces & (1ULL << Board::Squares::d3 | 1ULL << Board::Squares::e3) & gamestate.w_pawn << 8);

    return value;
}

int Evaluator::EvaluateStructure() {
    Gamestate& gamestate = Gamestate::Get();
    U64 open_files = 0, w_half_open = 0, b_half_open = 0, file;
    int value = 0;

    for (int shift = 0; shift < 8; ++shift) {
        file = Board::Files::aFile << shift;

        if (!(file & gamestate.w_pawn)) {
            w_half_open ^= file;
        }
        if (!(file & gamestate.b_pawn)) {
            b_half_open ^= file;
        }
    }
    open_files = w_half_open & b_half_open;

    value += 10 * bit_cnt(gamestate.w_rook & w_half_open);
    value -= 10 * bit_cnt(gamestate.b_rook & b_half_open);
    value += 20 * bit_cnt(gamestate.w_rook & open_files);
    value -= 20 * bit_cnt(gamestate.b_rook & open_files);

    return value;
}

Evaluator::Evaluator() {

}

std::array<int, 64> FlipTable(const std::array<int, 64> table) {
    std::array<int, 64> flippedTable;
    int square = 0, row, col;

    for (; square < 64; square++) {
        row = square / 8;
        col = square % 8;
        flippedTable[square] = table[8 * (7 - row) + col];
    }
    return flippedTable;
}

std::array<int, 64> NegateTable(const std::array<int, 64> table) {
    std::array<int, 64> negativeTable;
    int square = 0;

    for (; square < 64; ++square) {
        negativeTable[square] = -table[square];
    }
    return negativeTable;
}