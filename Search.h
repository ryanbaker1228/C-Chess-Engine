//
// Created by Ryan Baker on 7/29/23.
//

#ifndef CHESS_ENGINE_SEARCH_H
#define CHESS_ENGINE_SEARCH_H

#include "gamestate.h"
#include "movegen.h"
#include "move.h"

class MovePicker {
public:
    MovePicker(const Gamestate& gamestate);

    void Search();
private:
    int bestEvaluation;

};

#endif //CHESS_ENGINE_SEARCH_H
