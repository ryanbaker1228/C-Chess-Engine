//
// Created by Ryan Baker on 8/18/23.
//

#ifndef CHESS_ENGINE_ZOBRIST_H
#define CHESS_ENGINE_ZOBRIST_H


#include <array>

#include "gamestate.h"


class Zobrist {
private:
    Zobrist();

    U64 randBit15();

public:
    static Zobrist& Get() {
        static Zobrist instance;
        return instance;
    }

    U64 GenerateKey();

    std::array<std::array<U64, 12>, 64> pieceKeys;
    std::array<U64, 16> castlingKeys;
    std::array<U64, 8> enPassantKeys;
    U64 whiteToMoveKey;
};


#endif //CHESS_ENGINE_ZOBRIST_H
