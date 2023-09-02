//
// Created by Ryan Baker on 8/11/23.
//

#ifndef CHESS_ENGINE_TRANSPOSITION_H
#define CHESS_ENGINE_TRANSPOSITION_H


#include "move.h"
#include "gamestate.h"
#include "Zobrist.h"
#include <random>
#include <unordered_map>


enum EvaluationType {Exact, BestCase, WorstCase};
const int LookUpFailed = INT32_MIN;

inline bool isMateEval(int eval) {
    return std::abs(eval) > INT32_MAX - 999;
}

class TranspositionTable {
private:
    TranspositionTable();

    int AdjustLookupMateEval(int eval, int depthFromRoot);
    int AdjustStoredMateEval(int eval, int depthFromRoot);

    struct Entry {
        int evaluation;
        EvaluationType evalType;
        Move bestMove;
        int depth;

        bool isInitialized = false;
    };

public:
    static TranspositionTable& Get() {
        static TranspositionTable instance;
        return instance;
    }

    int Lookup(int searchDepth, int depthFromRoot, int alpha, int beta);
    void StorePosition(int depth, int depthFromRoot, int evaluation, EvaluationType type, Move move);

    std::unordered_map<U64, Entry> positions;

    bool useTable = false;
};



#endif //CHESS_ENGINE_TRANSPOSITION_H
