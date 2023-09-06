//
// Created by Ryan Baker on 8/4/23.
//

#include "Bot.h"
#include "Search.h"
#include "Transposition.h"
#include "gamestate.h"
#include "evaluation.h"
#include "GUI/gui.h"


Bot::Bot() {

}

void Bot::PlayMove() {
    if (!bot_to_play || Gamestate::Get().whiteToMove != playing_white and false) return;

    MovePicker::Get().InitSearch();
    std::cout << PGNNotation(MovePicker::Get().bestMove) << ", ";
    Gamestate::Get().MakeMove(MovePicker::Get().bestMove);
    GUI::Get().UpdateHighlights();
}