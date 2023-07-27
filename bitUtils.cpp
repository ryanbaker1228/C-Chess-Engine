//
// Created by Ryan Baker on 7/25/23.
//

#include "bitUtils.h"


int BitUtils::getLSB(U64 number) {
    return static_cast<int>(log2(number & -number));
}

U64 BitUtils::getMSB(U64 number) {
    int leading_zero_count = 63 - static_cast<int>(floor(log2(number | 1)));
    return (1ULL << 63) >> leading_zero_count;
}

int BitUtils::popLSB(U64& number) {
    int least_significant_bit = static_cast<int>(log2(number & -number));
    number &= number - 1;
    return least_significant_bit;
}

std::vector<int> BitUtils::getBits(U64 number) {
    std::vector<int> sqs;
    while (number) {
        sqs.push_back(static_cast<int>(log2(number & -number)));
        number &= number - 1;
    }
    return sqs;
}

int BitUtils::countBits(U64 number) {
    int count = 0;
    while (number) {
        number &= (number - 1);
        count++;
    }
    return count;
}

U64 BitMasks::segmentMask(int fromSquare, int toSquare) {
    U64 mask = 0;
    int fromRow = fromSquare / 8,
            toRow = toSquare / 8,
            fromCol = fromSquare % 8,
            toCol = toSquare % 8;
    int segmentLength = std::max(std::abs(toRow - fromRow), std::abs(toCol - fromCol));
    int horizontalDirection = (toCol - fromCol) / segmentLength;
    int verticalDirection = (toRow - fromRow) / segmentLength;
    int square;

    for (int distance = 1; distance <= segmentLength; ++distance) {
        square = 8 * (fromRow + distance * verticalDirection) + fromCol + distance * horizontalDirection;
        mask |= 1ULL << square;
    }
    return mask;
}

U64 BitMasks::xRay(int fromSquare, int thruSquare, U64 occSquares) {
    int fromRow = fromSquare / 8,
            thruRow = thruSquare / 8,
            fromCol = fromSquare % 8,
            thruCol = thruSquare % 8;
    int segmentLength = std::max(std::abs(thruRow - fromRow), std::abs(thruCol - fromCol));
    int horizontalDirection = (thruCol - fromCol) / segmentLength;
    int verticalDirection = (thruRow - fromRow) / segmentLength;
    int row = verticalDirection + thruRow, col = horizontalDirection + thruCol, square;

    while (true) {
        square = 8 * row + col;
        if (row < 0 || row > 7 || col < 0 || col > 7) {
            return 0;
        }
        if ((1ULL << square) & occSquares) {
            return 1ULL << square;
        }
        row += verticalDirection;
        col += horizontalDirection;
    }
}