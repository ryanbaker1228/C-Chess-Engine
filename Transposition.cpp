//
// Created by Ryan Baker on 8/11/23.
//

#include "Transposition.h"
#include "Search.h"


TranspositionTable::TranspositionTable() {

}

int TranspositionTable::Lookup(int searchDepth, int depthFromRoot, int alpha, int beta) {
    if (!useTable) return LookUpFailed;

    Gamestate::Get().zobristKey = Zobrist::Get().GenerateKey();
    Entry position = positions[Gamestate::Get().zobristKey];

    if (!position.isInitialized || position.depth < searchDepth) {
        return LookUpFailed;
    }

    int adjustedScore = AdjustLookupMateEval(position.evaluation, depthFromRoot);

    if (position.evalType == Exact) {
        return adjustedScore;
    }
    if (position.evalType == BestCase && adjustedScore <= alpha) {
        return adjustedScore;
    }
    if (position.evalType == WorstCase && adjustedScore >= beta) {
        return adjustedScore;
    }
    return LookUpFailed;
}

void TranspositionTable::StorePosition(int depth, int depthFromRoot, int evaluation, EvaluationType type, Move move) {
    if (!useTable) return;

    Gamestate::Get().zobristKey = Zobrist::Get().GenerateKey();
    Entry* position = &positions[Gamestate::Get().zobristKey];

    evaluation = AdjustStoredMateEval(evaluation, depthFromRoot);

    position->evaluation = evaluation;
    position->evalType = type;
    position->bestMove = move;
    position->depth = depth;
    position->isInitialized = true;
    position->age = Gamestate::Get().moveLog.size();
}


int TranspositionTable::AdjustLookupMateEval(int eval, int depthFromRoot) {
    if (isMateEval(eval)) {
        int sign = eval > 0 ? 1 : -1;
        eval = (eval * sign - depthFromRoot) * sign;
    }
    return eval;
}

int TranspositionTable::AdjustStoredMateEval(int eval, int depthFromRoot) {
    if (isMateEval(eval)) {
        int sign = eval > 0 ? 1 : -1;
        eval = (eval * sign + depthFromRoot) * sign;
    }
    return eval;
}
