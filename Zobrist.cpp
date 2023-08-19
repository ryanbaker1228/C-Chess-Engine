//
// Created by Ryan Baker on 8/18/23.
//

#include "Zobrist.h"

Zobrist::Zobrist() {
    srand(0);

    for (int square = 0; square < 64; ++square) {
        for (int piece = 0; piece < 12; ++piece) {
            pieceKeys[square][piece] = randBit15() << 48 ^
                                       randBit15() << 35 ^
                                       randBit15() << 22 ^
                                       randBit15() << 9 ^
                                       randBit15() >> 4;
        }
    }

    for (int key = 0; key < 16; ++key) {
        castlingKeys[key] = randBit15() << 48 ^
                            randBit15() << 35 ^
                            randBit15() << 22 ^
                            randBit15() << 9 ^
                            randBit15() >> 4;
    }

    for (int key = 0; key < 8; ++key) {
        enPassantKeys[key] = randBit15() << 48 ^
                             randBit15() << 35 ^
                             randBit15() << 22 ^
                             randBit15() << 9 ^
                             randBit15() >> 4;
    }

    whiteToMoveKey = randBit15() << 48 ^
                     randBit15() << 35 ^
                     randBit15() << 22 ^
                     randBit15() << 9 ^
                     randBit15() >> 4;
}

U64 Zobrist::GenerateKey() {
    Gamestate& gamestate = Gamestate::Get();
    U64 key = 0;

    for (int square = 0; square < 64; ++square) {
        if (gamestate.mailbox[square] != 0) {
            key ^= pieceKeys[square][PieceNum2BitboardIndex.at(gamestate.mailbox[square])];
        }
    }

    key ^= castlingKeys[gamestate.legality & legalityBits::castleMask];

    if (gamestate.legality & legalityBits::enPassantLegalMask) {
        key ^= enPassantKeys[(gamestate.legality & legalityBits::enPassantFileMask) >> legalityBits::enPassantFileShift];
    }

    if (gamestate.whiteToMove) key ^= whiteToMoveKey;

    return key;
}

U64 Zobrist::randBit15() {
    return (U64)rand() & 0x7fff;
}
