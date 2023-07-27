//
// Created by Ryan Baker on 7/25/23.
//

#ifndef CHESS_ENGINE_BITUTILS_H
#define CHESS_ENGINE_BITUTILS_H

#include <cstdint>
#include <vector>

typedef uint64_t U64;

namespace BitUtils {
    int getLSB(U64 number);
    U64 getMSB(U64 number);
    int popLSB(U64& number);

    std::vector<int> getBits(U64 number);
    int countBits(U64 number);
}

namespace BitMasks {
    U64 segmentMask(int fromSquare, int toSquare);
    U64 xRay(int fromSquare, int thruSquare, U64 occSquares);
}

#endif //CHESS_ENGINE_BITUTILS_H
