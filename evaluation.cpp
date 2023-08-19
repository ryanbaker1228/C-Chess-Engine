//
// Created by Ryan Baker on 7/11/23.
//

#include "evaluation.h"
#include "bitUtils.h"
#include "movegen.h"


int Evaluator::StaticEvaluation() {
    ++callCount;
    CountMaterial();
    int eval = material + EvaluatePcSqTables() + MopUpEvaluation();
    return (Gamestate::Get().whiteToMove ? eval : -eval);
}



void Evaluator::CountMaterial() {
    Gamestate& gamestate = Gamestate::Get();
    material = 0;

    for (int piece = 0; piece < 12; ++piece) {
        material += (1 - gamestate.gamePhase) * BitUtils::countBits(*gamestate.bitboards[piece]) * PieceValues::midGameValues[piece] +
                          gamestate.gamePhase * BitUtils::countBits(*gamestate.bitboards[piece]) * PieceValues::endGameValues[piece];
    }
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

    if (Gamestate::Get().gamePhase > 0.75 && (Gamestate::Get().whiteToMove ? material > 500 : material < -500)) {
        value += 10 * PcSqTables::centerManhattanDistance[squareOf(EnemyKing())];
        value -= 4 * ManhattanDistance(squareOf(FriendlyKing()), squareOf(EnemyKing()));
    }

    return Gamestate::Get().whiteToMove ? value : -value;
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