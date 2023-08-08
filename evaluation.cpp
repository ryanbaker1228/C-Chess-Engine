//
// Created by Ryan Baker on 7/11/23.
//

#include "evaluation.h"
#include "bitUtils.h"
#include "movegen.h"


int Evaluator::StaticEvaluation() {
    return (CountMaterial() + EvaluatePcSqTables());
}



int Evaluator::CountMaterial() {
    Gamestate& gamestate = Gamestate::Get();
    int material = 0;

    for (int piece = 0; piece < 12; ++piece) {
        material += BitUtils::countBits(*gamestate.bitboards[piece]) * PieceValues::midGameValues[piece];
    }

    return material;
}

int Evaluator::EvaluatePcSqTables() {
    Gamestate& gamestate = Gamestate::Get();
    int value = 0;
    U64 bitboard;

    for (int piece = 0; piece < 12; ++piece) {
        bitboard = *gamestate.bitboards[piece];
        while (bitboard) value += PcSqTables::midGameTables[piece][popLSB(bitboard)];
    }
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