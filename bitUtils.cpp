//
// Created by Ryan Baker on 7/25/23.
//

#include "bitUtils.h"



std::vector<int> BitUtils::getBits(U64 number) {
    std::vector<int> sqs;
    while (number) {
        sqs.push_back(static_cast<int>(log2(number & -number)));
        number &= number - 1;
    }
    return sqs;
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