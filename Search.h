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

public:
    static MovePicker& Get() {
        static MovePicker instance;
        return instance;
    }
    void InitSearch();
    int MiniMaxSearch(int depth, int alpha, int beta);

    Move bestMove;
    int bestEval;
};

#endif //CHESS_ENGINE_SEARCH_H
