//
// Created by Ryan Baker on 7/23/23.
//

#include "move.h"
#include "gamestate.h"


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

std::string PGNNotation(Move move) {
    if (move.flag == MoveFlags::longCastle) return "O-O-O";
    if (move.flag == MoveFlags::shortCastle) return "O-O";

    Gamestate& gamestate = Gamestate::Get();
    std::string notation = "";

    int moving_piece = gamestate.mailbox[move.startSquare];
    notation += PieceNum2Char.at(moving_piece);
    notation += AlgebraicNotation(move);

    switch (move.flag) {
        case MoveFlags::knightPromoCapt:
        case MoveFlags::knightPromotion:
            notation += "=N";

        case MoveFlags::bishopPromoCapt:
        case MoveFlags::bishopPromotion:
            notation += "=B";

        case MoveFlags::rookPromoCapt:
        case MoveFlags::rookPromotion:
            notation += "=R";

        case MoveFlags::queenPromoCapt:
        case MoveFlags::queenPromotion:
            notation += "=Q";
    }

    return notation;
}
