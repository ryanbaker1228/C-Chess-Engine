//
// Created by Ryan Baker on 7/10/23.
//

#ifndef CHESS_ENGINE_MOVE_H
#define CHESS_ENGINE_MOVE_H

#include <string>

namespace MoveFlags {
    const int quietMove = 0;
    const int doublePawnPush = 1;
    const int shortCastle = 2;
    const int longCastle = 3;
    const int capture = 4;
    const int enPassant = 5;
    const int knightPromotion = 8;
    const int bishopPromotion = 9;
    const int rookPromotion = 10;
    const int queenPromotion = 11;
    const int knightPromoCapt = 12;
    const int bishopPromoCapt = 13;
    const int rookPromoCapt = 14;
    const int queenPromoCapt = 15;

    const int promotion = 8;

    const int nullMove = -1;
}

class Move {
public:
    int startSquare, endSquare, moveFlag;

    Move(int fromSquare = 0, int toSquare = 0, int flag = MoveFlags::nullMove);
};

std::string AlgebraicNotation(Move move);

#endif //CHESS_ENGINE_MOVE_H
