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
    void InitSearch();

    Move bestMove;
    int eval;

public:
    static MovePicker& Get() {
        static MovePicker instance;
        return instance;
    }
};

#endif //CHESS_ENGINE_SEARCH_H
