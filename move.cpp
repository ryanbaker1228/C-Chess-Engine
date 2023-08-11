//
// Created by Ryan Baker on 7/23/23.
//

#include "move.h"


Move::Move(int fromSquare, int toSquare, int moveFlag) {
    startSquare = fromSquare;
    endSquare = toSquare;
    flag = moveFlag;
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

    std::string startSquareAlgebraic = squareToAlgebraic(move.startSquare);
    std::string endSquareAlgebraic = squareToAlgebraic(move.endSquare);

    return startSquareAlgebraic + endSquareAlgebraic;
}
