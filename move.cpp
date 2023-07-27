//
// Created by Ryan Baker on 7/23/23.
//

#include "move.h"
#include "evaluation.h"

Move::Move(int from_sq, int to_sq, GAMESTATE *gamestate, int moveFlag) {
    start_sq = from_sq;
    end_sq = to_sq;
    moving_piece = gamestate->mailbox[from_sq];
    captured_piece = gamestate->mailbox[to_sq];
    sqs = 1ULL << from_sq | 1ULL << to_sq;
    flags = moveFlag;
    flags |= MoveFlags::capture * (gamestate->mailbox[to_sq] > 0);
    //gamestate->makeMove(*this);
    //promise = StaticEvaluate(gamestate);
    //gamestate->undoMove();
}

bool Move::operator < (Move move) {
    return (promise < move.promise);
}

bool Move::operator == (Move move) {
    return (promise == move.promise);
}

std::string AlgebraicNotation(Move move) {
    auto squareToAlgebraic = [](int square) -> std::string {
        std::string algebraic = "";
        char col = 'a' + (square % 8);
        int row = (square / 8) + 1;
        algebraic += col;
        algebraic += std::to_string(row);
        return algebraic;
    };

    std::string startSquareAlgebraic = squareToAlgebraic(move.start_sq);
    std::string endSquareAlgebraic = squareToAlgebraic(move.end_sq);

    return startSquareAlgebraic + endSquareAlgebraic;
}
