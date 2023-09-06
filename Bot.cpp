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
    Evaluator::Get().callCount = 0;
    MovePicker::Get().InitSearch();
    call_count += Evaluator::Get().callCount;
    total_time += 1;
    //std::cout << (call_count / total_time) << std:: endl;
    std::cout << PGNNotation(MovePicker::Get().bestMove) << ", ";
    Gamestate::Get().MakeMove(MovePicker::Get().bestMove);
    GUI::Get().UpdateHighlights();
}