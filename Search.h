//
// Created by Ryan Baker on 7/29/23.
//

#ifndef CHESS_ENGINE_SEARCH_H
#define CHESS_ENGINE_SEARCH_H

#include "gamestate.h"
#include "movegen.h"
#include "move.h"

namespace constantEvals {
    const int whiteWin = 999999;
    const int positiveInfinity = 999999;
    const int blackWin = -999999;
    const int negativeInfinity = -999999;
    const int draw = 0;
}

class MovePicker {
private:
    MovePicker();
    int maxDepth = 32;
public:
    static MovePicker& Get() {
        static MovePicker instance;
        return instance;
    }
    void InitSearch();
    int MiniMaxSearch(int depth, int depthFromRoot, int alpha, int beta);
    int NegaMaxSearch(int depth, int depthFromRoot, int alpha, int beta);
    int QuiescenceSearch();
    void IterativeDeepeningSearch();

    Move bestMove;
    int bestEval;
};

class MoveOrderer {
private:
    MoveOrderer();
    std::array<int, 64> GuardValues;

    void ComputeGuardHeuristic();
    U64 enemyPawnAttacks;

    int currentDepth;

    const std::unordered_map<int, int> GuardScores = {
            {1, 900},
            {2, 550},
            {3, 500},
            {4, 200},
            {5, 100},
            {6, 50},
            {9, 900},
            {10, 550},
            {11, 500},
            {12, 200},
            {13, 100},
            {14, 50},
    };

    const std::unordered_map<int, int> MoveFlagPromise = {
            {MoveFlags::quietMove, 0},
            {MoveFlags::doublePawnPush, 10},
            {MoveFlags::shortCastle, 40},
            {MoveFlags::longCastle, 35},
            {MoveFlags::capture, 100},
            {MoveFlags::enPassant, 110},
            {MoveFlags::knightPromotion, 500},
            {MoveFlags::bishopPromotion, 400},
            {MoveFlags::rookPromotion, 450},
            {MoveFlags::queenPromotion, 600},
            {MoveFlags::knightPromoCapt, 520},
            {MoveFlags::bishopPromoCapt, 420},
            {MoveFlags::rookPromoCapt, 470},
            {MoveFlags::queenPromoCapt, 620},
    };

    const std::unordered_map<int, PieceType> PromotingPiece = {
            {MoveFlags::knightPromotion, knight},
            {MoveFlags::knightPromoCapt, knight},
            {MoveFlags::bishopPromotion, bishop},
            {MoveFlags::bishopPromoCapt, bishop},
            {MoveFlags::rookPromotion, rook},
            {MoveFlags::rookPromoCapt, rook},
            {MoveFlags::queenPromotion, queen},
            {MoveFlags::queenPromoCapt, queen},
    };
public:
    static MoveOrderer& Get() {
        static MoveOrderer instance;
        return instance;
    }
    int c = 0;
    void OrderMoves(int depth);
    int Promise(Move move);
};

inline bool operator<(const Move& first, const Move& second) {
    return MoveOrderer::Get().Promise(first) < MoveOrderer::Get().Promise(second);
}

inline bool operator==(const Move& first, const Move& second) {
    return MoveOrderer::Get().Promise(first) == MoveOrderer::Get().Promise(second);
}

inline bool operator>(const Move& first, const Move& second) {
    return MoveOrderer::Get().Promise(first) > MoveOrderer::Get().Promise(second);
}

#endif //CHESS_ENGINE_SEARCH_H
