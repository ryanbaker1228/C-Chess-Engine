//
// Created by Ryan Baker on 7/11/23.
//

#include "evaluation.h"
#include "bitUtils.h"
#include <cstdlib>

int Evaluator::StaticEvaluation() {
    gamePhase = 0.5;
    return 0;
}

int Evaluator::MaterialEvaluation() {
    return PcSqTables::mid
}

int StaticEvaluate(Gamestate* gamestate) {
    int centipawnEval = 0;
    // gamePhase is between 0 and 1, represents the stage of the game, near 1 indicates the opening, near 0 is the endgame
    float gamePhase = float(BitUtils::countBits(gamestate->all_pieces)) / 32;

    for (int square = 0; square < 64; square++) {
        if (!(gamestate->mailbox[square])) continue;
        centipawnEval += gamePhase * PcSqTables::midGameTables[PIECE_NUM_TO_ARRAY_INDEX.at(gamestate->mailbox[square])][square] +
                   (1 - gamePhase) * PcSqTables::endGameTables[PIECE_NUM_TO_ARRAY_INDEX.at(gamestate->mailbox[square])][square] +
                         gamePhase * PieceValues::midGameValues[PIECE_NUM_TO_ARRAY_INDEX.at(gamestate->mailbox[square])] +
                   (1 - gamePhase) * PieceValues::endGameValues[PIECE_NUM_TO_ARRAY_INDEX.at(gamestate->mailbox[square])];
    }
    return centipawnEval;
}

int Evaluator::CountMaterial() {
    Gamestate& gamestate = Gamestate::Get();
    int material = 0;

    material += std::bitset::count(gamestate.w_pawn).
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

    for (; square < 64; square++) {
        negativeTable[square] = -table[square];
    }
    return negativeTable;
}