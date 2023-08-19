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
        position.evaluation > 0 ?
            position.evaluation = Infinity - depthFromRoot :
            position.evaluation = -(Infinity - depthFromRoot);
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
    Entry position = positions[Gamestate::Get().zobristKey];

    position.evaluation = evaluation;
    position.evalType = type;
    position.bestMove = move;
    position.depth = depth;
    position.isInitialized = true;
}
