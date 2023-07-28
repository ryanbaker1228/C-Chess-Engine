//
// Created by Ryan Baker on 7/25/23.
//

#ifndef CHESS_ENGINE_BITUTILS_H
#define CHESS_ENGINE_BITUTILS_H

#include <cstdint>
#include <vector>

typedef uint64_t U64;

namespace BitUtils {
    inline int getLSB(U64 number) {
        return static_cast<int>(log2(number & -number));
    }

    inline U64 getMSB(U64 number) {
        int leading_zero_count = 63 - static_cast<int>(floor(log2(number | 1)));
        return (1ULL << 63) >> leading_zero_count;
    }

    inline int popLSB(U64& number) {
        int least_significant_bit = static_cast<int>(log2(number & -number));
        number &= number - 1;
        return least_significant_bit;
    }

    std::vector<int> getBits(U64 number);
    int countBits(U64 number);
}

namespace BitMasks {
    U64 segmentMask(int fromSquare, int toSquare);
    U64 xRay(int fromSquare, int thruSquare, U64 occSquares);
}

#endif //CHESS_ENGINE_BITUTILS_H
