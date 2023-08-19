//
// Created by Ryan Baker on 8/11/23.
//

#include "Transposition.h"
#include "Search.h"


TranspositionTable::TranspositionTable() {

}

int TranspositionTable::Lookup(int searchDepth, int depthFromRoot, int alpha, int beta) {
    Gamestate::Get().zobristKey = Zobrist::Get().GenerateKey();
    Entry position = positions[Gamestate::Get().zobristKey];

    if (!position.isInitialized || position.depth < searchDepth) {
        return LookUpFailed;
    }

    if (isMateEval(position.evaluation)) {
        if (position.evaluation > 0) {
            position.evaluation = Infinity - searchDepth;
        } else {
            position.evaluation = -(Infinity - searchDepth);
        }
    }

    if (position.evalType == BestCase && position.evaluation > alpha) {
        return LookUpFailed;
    }

    if (position.evalType == WorstCase && position.evaluation < beta) {
        return LookUpFailed;
    }

    return position.evaluation;
}

void TranspositionTable::StorePosition(int depth, int depthFromRoot, int evaluation, EvaluationType type, Move move) {
    Gamestate::Get().zobristKey = Zobrist::Get().GenerateKey();
    Entry* position = &positions[Gamestate::Get().zobristKey];

    if (isMateEval(evaluation)) {
        if (position->evaluation > 0) {
            position->evaluation = Infinity - depth;
        } else {
            position->evaluation = -(Infinity - depth);
        }
    }

    position->evaluation = evaluation;
    position->evalType = type;
    position->bestMove = move;
    position->depth = depth;
    position->isInitialized = true;
}
