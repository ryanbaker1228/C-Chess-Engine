//
// Created by Ryan Baker on 7/1/23.
//

#include "movegen.h"
#include "bitUtils.h"
#include <vector>
#include <cmath>


void MovementTables::LoadTables() {
    /* Load knight move table */
    const int knightDirections[8][2] = {{-1, 2}, {1, 2}, {2, 1}, {2, -1}, {1, -2}, {-1, -2}, {-2, -1}, {-2, 1}};
    int fromSquare = 0, toSquare, fromRow, toRow, fromCol, toCol;
    for(; fromSquare < 64; fromSquare++) {
        knightMoves[fromSquare] = 0;
        fromRow = fromSquare / 8;
        fromCol = fromSquare % 8;
        for(auto* direction : knightDirections) {
            toRow = fromRow + direction[0];
            toCol = fromCol + direction[1];
            toSquare = 8 * toRow + toCol;
            if (toRow >= 0 && toRow < 8 && toCol >= 0 && toCol < 8) knightMoves[fromSquare] |= 1ULL << toSquare;
        }
    }

    /* Load bishop move table */
    const int bishopDirections[4][2] = {{1, -1}, {1, 1}, {-1, 1}, {-1, -1}};
    fromSquare = 0;
    for (; fromSquare < 64; fromSquare++) {
        fromRow = fromSquare / 8;
        fromCol = fromSquare % 8;
        for (int direction = 0; direction < 4; direction++) {
            bishopMoves[fromSquare][direction] = 0;
            toRow = fromRow + bishopDirections[direction][0];
            toCol = fromCol + bishopDirections[direction][1];
            while (toRow >= 0 && toRow < 8 && toCol >= 0 && toCol < 8) {
                toSquare = 8 * toRow + toCol;
                bishopMoves[fromSquare][direction] |= 1ULL << toSquare;
                toRow += bishopDirections[direction][0];
                toCol += bishopDirections[direction][1];
            }
        }
        bishopMoves[fromSquare][4] = bishopMoves[fromSquare][0] | bishopMoves[fromSquare][2];
        bishopMoves[fromSquare][5] = bishopMoves[fromSquare][1] | bishopMoves[fromSquare][3];
        bishopMoves[fromSquare][6] = bishopMoves[fromSquare][4] | bishopMoves[fromSquare][5];
    }

    /* Load rook move table */
    const int rookDirections[4][2] = {{1, 0}, {0, 1}, {-1, 0}, {0, -1}};
    fromSquare = 0;
    for (; fromSquare < 64; fromSquare++) {
        fromRow = fromSquare / 8;
        fromCol = fromSquare % 8;
        for (int direction = 0; direction < 4; direction++) {
            rookMoves[fromSquare][direction] = 0;
            toRow = fromRow + rookDirections[direction][0];
            toCol = fromCol + rookDirections[direction][1];
            while (toRow >= 0 && toRow < 8 && toCol >= 0 && toCol < 8) {
                toSquare = 8 * toRow + toCol;
                rookMoves[fromSquare][direction] |= 1ULL << toSquare;
                toRow += rookDirections[direction][0];
                toCol += rookDirections[direction][1];
            }
        }
        rookMoves[fromSquare][4] = rookMoves[fromSquare][0] | rookMoves[fromSquare][2];
        rookMoves[fromSquare][5] = rookMoves[fromSquare][1] | rookMoves[fromSquare][3];
        rookMoves[fromSquare][6] = rookMoves[fromSquare][4] | rookMoves[fromSquare][5];
    }

    /* Load king move table */
    const int kingDirections[8][2] = {{-1, -1}, {0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}};
    fromSquare = 0;
    for(; fromSquare < 64; fromSquare++) {
        kingMoves[fromSquare] = 0;
        fromRow = fromSquare / 8;
        fromCol = fromSquare % 8;
        for(auto* direction : kingDirections) {
            toRow = fromRow + direction[0];
            toCol = fromCol + direction[1];
            toSquare = 8 * toRow + toCol;
            if (toRow >= 0 && toRow < 8 && toCol >= 0 && toCol < 8) kingMoves[fromSquare] |= 1ULL << toSquare;
        }
    }
}

MoveGenerator::MoveGenerator() {
    legalMoves.reserve(218);
}

std::vector<Move> MoveGenerator::GenerateLegalMoves(bool capturesOnly) {
    legalMoves.clear();

    CalculateEnemyAttacks();
    CalculateCheckMask();
    CalculatePinMasks();

    GenerateKingMoves();

    if (king_is_in_double_check) {
        king_is_in_check = true;
        return legalMoves;
    }
    king_is_in_check = FriendlyKing() & enemyAttacks;

    GeneratePawnMoves();
    GenerateKnightMoves();
    GenerateBishopMoves();
    GenerateRookMoves();

    std::vector<Move>::iterator move = legalMoves.begin();

    while(move != legalMoves.end()) {
        if (1ULL << move->startSquare & pinnedPieces && !(1ULL << move->endSquare & pinMasks[move->startSquare])) {
            move = legalMoves.erase(move);
            continue;
        }
        if (capturesOnly && !(move->flag & MoveFlags::capture)) {
            move = legalMoves.erase(move);
            continue;
        }
        ++move;
    }
    return legalMoves;
}

float MoveGenerator::CountLegalMoves() {
    float whiteCount = 0, blackCount = 0;

    Gamestate& gamestate = Gamestate::Get();

    U64 whiteBishops = gamestate.w_bishop | gamestate.w_queen;
    U64 blackBishops = gamestate.b_bishop | gamestate.b_queen;
    U64 whiteRooks = gamestate.w_rook | gamestate.w_queen;
    U64 blackRooks = gamestate.b_rook | gamestate.b_queen;
    U64 whiteKnights = gamestate.w_knight;
    U64 blackKnights = gamestate.b_knight;

    U64 northwest, southwest, northeast, southeast;
    U64 north, south, east, west;
    U64 most_significant_bit, difference, target_squares = 0;

    while (whiteBishops) {
        target_squares = 0;
        int bishop = popLSB(whiteBishops);

        northwest = gamestate.all_pieces & MovementTables::bishopMoves[bishop][0];
        southeast = gamestate.all_pieces & MovementTables::bishopMoves[bishop][2];
        northeast = gamestate.all_pieces & MovementTables::bishopMoves[bishop][1];
        southwest = gamestate.all_pieces & MovementTables::bishopMoves[bishop][3];

        most_significant_bit = getMSB(southeast);
        difference = northwest ^ (northwest - most_significant_bit);
        target_squares |= difference & MovementTables::bishopMoves[bishop][4] & ~gamestate.w_pieces;

        most_significant_bit = getMSB(southwest);
        difference = northeast ^ (northeast - most_significant_bit);
        target_squares |= difference & MovementTables::bishopMoves[bishop][5] & ~gamestate.w_pieces;

        whiteCount += bit_cnt(target_squares);
    }

    while (blackBishops) {
        target_squares = 0;
        int bishop = popLSB(blackBishops);

        northwest = gamestate.all_pieces & MovementTables::bishopMoves[bishop][0];
        southeast = gamestate.all_pieces & MovementTables::bishopMoves[bishop][2];
        northeast = gamestate.all_pieces & MovementTables::bishopMoves[bishop][1];
        southwest = gamestate.all_pieces & MovementTables::bishopMoves[bishop][3];

        most_significant_bit = getMSB(southeast);
        difference = northwest ^ (northwest - most_significant_bit);
        target_squares |= difference & MovementTables::bishopMoves[bishop][4] & ~gamestate.b_pieces;

        most_significant_bit = getMSB(southwest);
        difference = northeast ^ (northeast - most_significant_bit);
        target_squares |= difference & MovementTables::bishopMoves[bishop][5] & ~gamestate.b_pieces;

        blackCount += bit_cnt(target_squares);
    }

    while(whiteRooks) {
        target_squares = 0;
        int rook = popLSB(whiteRooks);

        north = gamestate.all_pieces & MovementTables::rookMoves[rook][0];
        south = gamestate.all_pieces & MovementTables::rookMoves[rook][2];
        east = gamestate.all_pieces & MovementTables::rookMoves[rook][1];
        west = gamestate.all_pieces & MovementTables::rookMoves[rook][3];

        most_significant_bit = getMSB(south);
        difference = north ^ (north - most_significant_bit);
        target_squares |= difference & MovementTables::rookMoves[rook][4] & ~gamestate.w_pieces;

        most_significant_bit = getMSB(west);
        difference = east ^ (east - most_significant_bit);
        target_squares |= difference & MovementTables::rookMoves[rook][5] & ~gamestate.w_pieces;

        whiteCount += bit_cnt(target_squares);
    }

    while(blackRooks) {
        target_squares = 0;
        int rook = popLSB(blackRooks);

        north = gamestate.all_pieces & MovementTables::rookMoves[rook][0];
        south = gamestate.all_pieces & MovementTables::rookMoves[rook][2];
        east = gamestate.all_pieces & MovementTables::rookMoves[rook][1];
        west = gamestate.all_pieces & MovementTables::rookMoves[rook][3];

        most_significant_bit = getMSB(south);
        difference = north ^ (north - most_significant_bit);
        target_squares |= difference & MovementTables::rookMoves[rook][4] & ~gamestate.b_pieces;

        most_significant_bit = getMSB(west);
        difference = east ^ (east - most_significant_bit);
        target_squares |= difference & MovementTables::rookMoves[rook][5] & ~gamestate.b_pieces;

        blackCount += bit_cnt(target_squares);
    }

    while (whiteKnights) {
        whiteCount += bit_cnt(MovementTables::knightMoves[popLSB(whiteKnights)] & ~gamestate.w_pieces);
    }

    while (blackKnights) {
        blackCount += bit_cnt(MovementTables::knightMoves[popLSB(blackKnights)] & ~gamestate.b_pieces);
    }

    return (whiteCount + 1) / (blackCount + 1);
}

void MoveGenerator::CalculateEnemyAttacks() {
    const Gamestate& gamestate = Gamestate::Get();

    U64 friendlyPieces, kingPos, orthogonalSliders, diagonalSliders, enemyKnights, allPieces,
            north, south, east, west, northeast, northwest, southeast, southwest, mostSignificantBit, difference;
    int slider;

    if (gamestate.whiteToMove) {
        enemyAttacks = (*gamestate.bitboards[6] & ~Board::Files::hFile) >> 7 | (*gamestate.bitboards[6] & ~Board::Files::aFile) >> 9;
        friendlyPieces = gamestate.w_pieces;
        kingPos = gamestate.w_king;
    } else {
        enemyAttacks = (*gamestate.bitboards[0] & ~Board::Files::aFile) << 7 | (*gamestate.bitboards[0] & ~Board::Files::hFile) << 9;
        friendlyPieces = gamestate.b_pieces;
        kingPos = gamestate.b_king;
    }

    allPieces = gamestate.all_pieces ^ kingPos;
    enemyKnights = (*gamestate.bitboards[1] | *gamestate.bitboards[7]) & ~friendlyPieces;
    diagonalSliders = (*gamestate.bitboards[2] | *gamestate.bitboards[4] | *gamestate.bitboards[8] | *gamestate.bitboards[10]) & ~friendlyPieces;
    orthogonalSliders =(*gamestate.bitboards[3] | *gamestate.bitboards[4] | *gamestate.bitboards[9] | *gamestate.bitboards[10]) & ~friendlyPieces;

    while (enemyKnights) enemyAttacks |= MovementTables::knightMoves[popLSB(enemyKnights)];
    while (diagonalSliders) {
        slider = popLSB(diagonalSliders);

        northwest = allPieces & MovementTables::bishopMoves[slider][0];
        southeast = allPieces & MovementTables::bishopMoves[slider][2];
        northeast = allPieces & MovementTables::bishopMoves[slider][1];
        southwest = allPieces & MovementTables::bishopMoves[slider][3];

        mostSignificantBit = getMSB(southeast);
        difference = northwest ^ (northwest - mostSignificantBit);
        enemyAttacks |= difference & MovementTables::bishopMoves[slider][4];

        mostSignificantBit = getMSB(southwest);
        difference = northeast ^ (northeast - mostSignificantBit);
        enemyAttacks |= difference & MovementTables::bishopMoves[slider][5];
    }
    while (orthogonalSliders) {
        slider = popLSB(orthogonalSliders);

        north = allPieces & MovementTables::rookMoves[slider][0];
        south = allPieces & MovementTables::rookMoves[slider][2];
        east = allPieces & MovementTables::rookMoves[slider][1];
        west = allPieces & MovementTables::rookMoves[slider][3];

        mostSignificantBit = getMSB(south);
        difference = north ^ (north - mostSignificantBit);
        enemyAttacks |= difference & MovementTables::rookMoves[slider][4];

        mostSignificantBit = getMSB(west);
        difference = east ^ (east - mostSignificantBit);
        enemyAttacks |= difference & MovementTables::rookMoves[slider][5];
    }
    enemyAttacks |= MovementTables::kingMoves[squareOf(~friendlyPieces & (*gamestate.bitboards[5] | *gamestate.bitboards[11]))];
}

void MoveGenerator::CalculateCheckMask() {
    const Gamestate& gamestate = Gamestate::Get();

    U64 kingPos;
    kingPos = gamestate.whiteToMove ? gamestate.w_king : gamestate.b_king;
    int kingSquare = squareOf(kingPos);
    king_is_in_check = false;
    king_is_in_double_check = false;

    if (!(enemyAttacks & kingPos)) {
        checkMask = ~0;
        return;
    }

    U64 orthogonalSliders, diagonalSliders, mostSignificantBit, difference, attacks = 0,
            north, south, east, west, northeast, northwest, southeast, southwest;

    if (gamestate.whiteToMove) {
        checkMask = ((gamestate.b_pawn & ~Board::Files::hFile) >> 7 & kingPos) << 7 |
                    ((gamestate.b_pawn & ~Board::Files::aFile) >> 9 & kingPos) << 9;

        checkMask |= MovementTables::knightMoves[kingSquare] & gamestate.b_knight;

        diagonalSliders = gamestate.b_bishop | gamestate.b_queen;
        northwest = gamestate.all_pieces & MovementTables::bishopMoves[kingSquare][0];
        southeast = gamestate.all_pieces & MovementTables::bishopMoves[kingSquare][2];
        northeast = gamestate.all_pieces & MovementTables::bishopMoves[kingSquare][1];
        southwest = gamestate.all_pieces & MovementTables::bishopMoves[kingSquare][3];

        mostSignificantBit = getMSB(southeast);
        difference = northwest ^ (northwest - mostSignificantBit);
        attacks |= difference & MovementTables::bishopMoves[kingSquare][4];

        mostSignificantBit = getMSB(southwest);
        difference = northeast ^ (northeast - mostSignificantBit);
        attacks |= difference & MovementTables::bishopMoves[kingSquare][5];

        if (attacks & diagonalSliders) {
            if (checkMask) {
                king_is_in_double_check = true;
                checkMask = 0;
                return;
            }
            checkMask = BitMasks::segmentMask(kingSquare, squareOf(diagonalSliders & attacks));
        }

        attacks = 0;
        orthogonalSliders = gamestate.b_rook | gamestate.b_queen;
        north = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][0];
        south = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][2];
        east = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][1];
        west = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][3];

        mostSignificantBit = getMSB(south);
        difference = north ^ (north - mostSignificantBit);
        attacks |= difference & MovementTables::rookMoves[kingSquare][4];

        mostSignificantBit = getMSB(west);
        difference = east ^ (east - mostSignificantBit);
        attacks |= difference & MovementTables::rookMoves[kingSquare][5];

        if (attacks & orthogonalSliders) {
            if (checkMask) {
                king_is_in_double_check = true;
                checkMask = 0;
                return;
            }
            king_is_in_double_check = false;
            checkMask = BitMasks::segmentMask(kingSquare, squareOf(orthogonalSliders & attacks));
        }
    } else {
        checkMask = ((gamestate.w_pawn & ~Board::Files::hFile) << 9 & kingPos) >> 9 |
                    ((gamestate.w_pawn & ~Board::Files::aFile) << 7 & kingPos) >> 7;

        checkMask |= MovementTables::knightMoves[kingSquare] & gamestate.w_knight;

        diagonalSliders = gamestate.w_bishop | gamestate.w_queen;
        northwest = gamestate.all_pieces & MovementTables::bishopMoves[kingSquare][0];
        southeast = gamestate.all_pieces & MovementTables::bishopMoves[kingSquare][2];
        northeast = gamestate.all_pieces & MovementTables::bishopMoves[kingSquare][1];
        southwest = gamestate.all_pieces & MovementTables::bishopMoves[kingSquare][3];

        mostSignificantBit = getMSB(southeast);
        difference = northwest ^ (northwest - mostSignificantBit);
        attacks |= difference & MovementTables::bishopMoves[kingSquare][4];

        mostSignificantBit = getMSB(southwest);
        difference = northeast ^ (northeast - mostSignificantBit);
        attacks |= difference & MovementTables::bishopMoves[kingSquare][5];

        if (attacks & diagonalSliders) {
            if (checkMask) { // if checkMask is not 0 this indicates a double check
                king_is_in_double_check = true;
                checkMask = 0;
                return;
            }
            king_is_in_double_check = false;
            checkMask = BitMasks::segmentMask(kingSquare, squareOf(diagonalSliders & attacks));
        }

        attacks = 0;
        orthogonalSliders = gamestate.w_rook | gamestate.w_queen;
        north = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][0];
        south = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][2];
        east = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][1];
        west = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][3];

        mostSignificantBit = getMSB(south);
        difference = north ^ (north - mostSignificantBit);
        attacks |= difference & MovementTables::rookMoves[kingSquare][4];

        mostSignificantBit = getMSB(west);
        difference = east ^ (east - mostSignificantBit);
        attacks |= difference & MovementTables::rookMoves[kingSquare][5];

        if (attacks & orthogonalSliders) {
            if (checkMask) {
                king_is_in_double_check = true;
                checkMask = 0;
                return;
            }
            king_is_in_double_check = false;
            checkMask = BitMasks::segmentMask(kingSquare, squareOf(orthogonalSliders & attacks));
        }
    }
}

void MoveGenerator::CalculatePinMasks() {
    const Gamestate& gamestate = Gamestate::Get();

    U64 friendlyPieces, enemyDiagonalSliders, enemyOrthogonalSliders, mostSignificantBit, difference, attacks = 0,
            north, south, east, west, northeast, northwest, southeast, southwest, xRayedSquare, enPassantRank,
            potentialPins, kingPos;
    pinnedPieces = 0;

    if (gamestate.whiteToMove) {
        kingPos = gamestate.w_king;
        friendlyPieces = gamestate.w_pieces;
    } else /* black to move */ {
        kingPos = gamestate.b_king;
        friendlyPieces = gamestate.b_pieces;
    }
    int kingSquare = squareOf(kingPos), potentialPinnedPiece;

    enemyDiagonalSliders = (gamestate.w_bishop | gamestate.w_queen | gamestate.b_bishop | gamestate.b_queen) & ~friendlyPieces;
    enemyOrthogonalSliders =(gamestate.w_rook | gamestate.w_queen | gamestate.b_rook | gamestate.b_queen) & ~friendlyPieces;
    enPassantRank = gamestate.whiteToMove ? Board::Ranks::rank_5 : Board::Ranks::rank_4;

    northwest = gamestate.all_pieces & MovementTables::bishopMoves[kingSquare][0];
    southeast = gamestate.all_pieces & MovementTables::bishopMoves[kingSquare][2];
    northeast = gamestate.all_pieces & MovementTables::bishopMoves[kingSquare][1];
    southwest = gamestate.all_pieces & MovementTables::bishopMoves[kingSquare][3];

    mostSignificantBit = getMSB(southeast);
    difference = northwest ^ (northwest - mostSignificantBit);
    attacks |= difference & MovementTables::bishopMoves[kingSquare][4];
    mostSignificantBit = getMSB(southwest);
    difference = northeast ^ (northeast - mostSignificantBit);
    attacks |= difference & MovementTables::bishopMoves[kingSquare][5];

    potentialPins = attacks & friendlyPieces;
    while (potentialPins) {
        potentialPinnedPiece = popLSB(potentialPins);
        xRayedSquare = BitMasks::xRay(kingSquare, potentialPinnedPiece, gamestate.all_pieces);
        if (xRayedSquare & enemyDiagonalSliders) {
            pinnedPieces |= 1ULL << potentialPinnedPiece;
            pinMasks[potentialPinnedPiece] = BitMasks::segmentMask(kingSquare, squareOf(xRayedSquare));
        }
    }

    attacks = 0;
    north = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][0];
    south = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][2];
    east = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][1];
    west = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][3];

    mostSignificantBit = getMSB(south);
    difference = north ^ (north - mostSignificantBit);
    attacks |= difference & MovementTables::rookMoves[kingSquare][4];
    mostSignificantBit = getMSB(west);
    difference = east ^ (east - mostSignificantBit);
    attacks |= difference & MovementTables::rookMoves[kingSquare][5];

    potentialPins = attacks & friendlyPieces;
    while (potentialPins) {
        potentialPinnedPiece = popLSB(potentialPins);
        xRayedSquare = BitMasks::xRay(kingSquare, potentialPinnedPiece, gamestate.all_pieces);
        if (xRayedSquare & enemyOrthogonalSliders) {
            pinnedPieces |= 1ULL << potentialPinnedPiece;
            pinMasks[potentialPinnedPiece] = BitMasks::segmentMask(kingSquare, squareOf(xRayedSquare));
        }
    }

    if (!gamestate.moveLog.empty()) {
        if (gamestate.moveLog.top().flag & MoveFlags::doublePawnPush &&
            kingPos & enPassantRank &&
            friendlyPieces & (gamestate.w_pawn | gamestate.b_pawn) & enPassantRank &&
            enemyOrthogonalSliders & enPassantRank) {
            for (int potentialPinnedEnPassant : BitUtils::getBits(attacks & (gamestate.w_pawn | gamestate.b_pawn) & (east | west))) {
                xRayedSquare = BitMasks::xRay(kingSquare, potentialPinnedEnPassant, gamestate.all_pieces);
                U64 doubleXRayedSquare = BitMasks::xRay(potentialPinnedEnPassant, squareOf(xRayedSquare), gamestate.all_pieces);
                if ((gamestate.mailbox[squareOf(xRayedSquare)] ^ gamestate.mailbox[potentialPinnedEnPassant]) & 8 &&
                    std::abs(getLSB(xRayedSquare) % 8 - potentialPinnedEnPassant % 8) == 1 &&
                    doubleXRayedSquare & enemyOrthogonalSliders)  {
                    if (gamestate.whiteToMove) {
                        if (gamestate.mailbox[potentialPinnedEnPassant] & 8) {
                            pinnedPieces |= xRayedSquare;
                            pinMasks[squareOf(xRayedSquare)] = ~((1ULL << potentialPinnedEnPassant) << 8);
                        } else {
                            pinnedPieces |= 1ULL << potentialPinnedEnPassant;
                            pinMasks[potentialPinnedEnPassant] = ~(xRayedSquare << 8);
                        }
                    } else {
                        if (gamestate.mailbox[potentialPinnedEnPassant] & 8) {
                            pinnedPieces |= 1ULL << potentialPinnedEnPassant;
                            pinMasks[potentialPinnedEnPassant] = ~(xRayedSquare >> 8);
                        } else {
                            pinnedPieces |= xRayedSquare;
                            pinMasks[squareOf(xRayedSquare)] = ~((1ULL << potentialPinnedEnPassant) >> 8);
                        }
                    }
                }
            }
        }
    }
}

void MoveGenerator::GenerateKingMoves() {
    const Gamestate& gamestate = Gamestate::Get();

    int king_sq;
    U64 friendly_pieces, to_squares;
    if (gamestate.whiteToMove) {
        king_sq = squareOf(gamestate.w_king);
        friendly_pieces = gamestate.w_pieces;
        if (gamestate.legality & legalityBits::whiteShortCastleMask && gamestate.mailbox[Board::Squares::h1] == 4 && king_sq == Board::Squares::e1 &&
            gamestate.empty_sqs & 1ULL << Board::Squares::f1 && gamestate.empty_sqs & 1ULL << Board::Squares::g1 &&
            !(enemyAttacks & 1ULL << Board::Squares::e1) && !(enemyAttacks & 1ULL << Board::Squares::f1) && !(enemyAttacks & 1ULL << Board::Squares::g1)) {
            legalMoves.emplace_back(king_sq, Board::Squares::g1, MoveFlags::shortCastle);
        }
        if (gamestate.legality & legalityBits::whiteLongCastleMask && gamestate.mailbox[Board::Squares::a1] == 4 && king_sq == Board::Squares::e1 &&
            gamestate.empty_sqs & 1ULL << Board::Squares::d1 && gamestate.empty_sqs & 1ULL << Board::Squares::c1 && gamestate.empty_sqs & 1ULL << Board::Squares::b1 &&
            !(enemyAttacks & 1ULL << Board::Squares::e1) && !(enemyAttacks & 1ULL << Board::Squares::d1) && !(enemyAttacks & 1ULL << Board::Squares::c1)) {
            legalMoves.emplace_back(king_sq, Board::Squares::c1, MoveFlags::longCastle);
        }
    } else {
        king_sq = squareOf(gamestate.b_king);
        friendly_pieces = gamestate.b_pieces;
        if (gamestate.legality & legalityBits::blackShortCastleMask && gamestate.mailbox[Board::Squares::h8] == 12 && king_sq == Board::Squares::e8 &&
            gamestate.empty_sqs & 1ULL << Board::Squares::f8 && gamestate.empty_sqs & 1ULL << Board::Squares::g8 &&
            !(enemyAttacks & 1ULL << Board::Squares::e8) && !(enemyAttacks & 1ULL << Board::Squares::f8) && !(enemyAttacks & 1ULL << Board::Squares::g8)) {
            legalMoves.emplace_back(king_sq, Board::Squares::g8, MoveFlags::shortCastle);
        }
        if (gamestate.legality & legalityBits::blackLongCastleMask && gamestate.mailbox[Board::Squares::a8] == 12 && king_sq == Board::Squares::e8 &&
            gamestate.empty_sqs & 1ULL << Board::Squares::d8 && gamestate.empty_sqs & 1ULL << Board::Squares::c8 && gamestate.empty_sqs & 1ULL << Board::Squares::b8 &&
            !(enemyAttacks & 1ULL << Board::Squares::e8) && !(enemyAttacks & 1ULL << Board::Squares::d8) && !(enemyAttacks & 1ULL << Board::Squares::c8)) {
            legalMoves.emplace_back(king_sq, Board::Squares::c8, MoveFlags::longCastle);
        }
    }

    to_squares = MovementTables::kingMoves[king_sq] & ~friendly_pieces & ~enemyAttacks;
    while (to_squares) {
        int toSquare = popLSB(to_squares);
        legalMoves.emplace_back(king_sq, toSquare, isCapture(gamestate, toSquare));
    }
}

void MoveGenerator::GeneratePawnMoves() {
    const Gamestate& gamestate = Gamestate::Get();

    U64 pawnSquares, singleStep, doubleStep, leftCapt, rightCapt, promotion, promotionLeftCapt, promotionRightCapt;
    U64 singleStep_f, doubleStep_f, leftCapt_f, rightCapt_f, promotion_f, promotionLeftCapt_f, promotionRightCapt_f;
    U64 promotionRank, enPassantRank, oneStepRank;
    U64 enemyPieces, enPassant_f = 0ULL, enPassantSquare = 0ULL, enPassantCheckMask;
    int fromSq, toSq;

    if (gamestate.whiteToMove) {
        pawnSquares = gamestate.w_pawn;
        promotionRank = Board::Ranks::rank_7;
        enPassantRank = Board::Ranks::rank_6;
        oneStepRank = Board::Ranks::rank_3;
        enemyPieces = gamestate.b_pieces;
    } else /* black to move */ {
        pawnSquares = gamestate.b_pawn;
        promotionRank = Board::Ranks::rank_2;
        enPassantRank = Board::Ranks::rank_3;
        oneStepRank = Board::Ranks::rank_6;
        enemyPieces = gamestate.w_pieces;
    }

    singleStep = PawnMoves::oneStep(gamestate.whiteToMove, pawnSquares & ~promotionRank) & gamestate.empty_sqs & checkMask;
    doubleStep = PawnMoves::twoStep(gamestate.whiteToMove, pawnSquares & (PawnMoves::oneStep(!gamestate.whiteToMove, oneStepRank & gamestate.empty_sqs))) & gamestate.empty_sqs & checkMask;
    leftCapt = PawnMoves::leftwardCapt(gamestate.whiteToMove, pawnSquares & ~promotionRank) & enemyPieces & checkMask;
    rightCapt = PawnMoves::rightwardCapt(gamestate.whiteToMove, pawnSquares & ~promotionRank) & enemyPieces & checkMask;
    promotion = PawnMoves::oneStep(gamestate.whiteToMove, pawnSquares & promotionRank) & gamestate.empty_sqs & checkMask;
    promotionLeftCapt = PawnMoves::leftwardCapt(gamestate.whiteToMove, pawnSquares & promotionRank) & enemyPieces & checkMask;
    promotionRightCapt = PawnMoves::rightwardCapt(gamestate.whiteToMove, pawnSquares & promotionRank) & enemyPieces & checkMask;

    singleStep_f = PawnMoves::oneStep(!gamestate.whiteToMove, singleStep);
    doubleStep_f = PawnMoves::twoStep(!gamestate.whiteToMove, doubleStep);
    leftCapt_f = PawnMoves::leftwardCapt(!gamestate.whiteToMove, leftCapt);
    rightCapt_f = PawnMoves::rightwardCapt(!gamestate.whiteToMove, rightCapt);
    promotion_f = PawnMoves::oneStep(!gamestate.whiteToMove, promotion);
    promotionLeftCapt_f = PawnMoves::leftwardCapt(!gamestate.whiteToMove, promotionLeftCapt);
    promotionRightCapt_f = PawnMoves::rightwardCapt(!gamestate.whiteToMove, promotionRightCapt);

    if (gamestate.legality & legalityBits::enPassantLegalMask) {
        enPassantSquare = Board::Files::aFile << ((gamestate.legality & legalityBits::enPassantFileMask) >> legalityBits::enPassantFileShift) & enPassantRank;
        enPassantCheckMask = PawnMoves::oneStep(gamestate.whiteToMove, (checkMask & PawnMoves::oneStep(!gamestate.whiteToMove, enPassantSquare)));
        enPassant_f = PawnMoves::allCaptures(!gamestate.whiteToMove, enPassantSquare & (checkMask | enPassantCheckMask)) & pawnSquares;
    }

    while (singleStep) legalMoves.emplace_back(popLSB(singleStep_f), popLSB(singleStep), MoveFlags::quietMove);
    while (doubleStep) legalMoves.emplace_back(popLSB(doubleStep_f), popLSB(doubleStep), MoveFlags::doublePawnPush);
    while (leftCapt) legalMoves.emplace_back(popLSB(leftCapt_f), popLSB(leftCapt), MoveFlags::capture);
    while (rightCapt) legalMoves.emplace_back(popLSB(rightCapt_f), popLSB(rightCapt), MoveFlags::capture);
    while (enPassant_f) legalMoves.emplace_back(popLSB(enPassant_f), squareOf(enPassantSquare), MoveFlags::enPassant);
    while (promotion) {
        fromSq = popLSB(promotion_f);
        toSq = popLSB(promotion);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::knightPromotion);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::bishopPromotion);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::rookPromotion);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::queenPromotion);
    }
    while (promotionLeftCapt) {
        fromSq = popLSB(promotionLeftCapt_f);
        toSq = popLSB(promotionLeftCapt);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::knightPromoCapt);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::bishopPromoCapt);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::rookPromoCapt);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::queenPromoCapt);
    }
    while (promotionRightCapt) {
        fromSq = popLSB(promotionRightCapt_f);
        toSq = popLSB(promotionRightCapt);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::knightPromoCapt);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::bishopPromoCapt);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::rookPromoCapt);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::queenPromoCapt);
    }
}

void MoveGenerator::GenerateKnightMoves() {
    const Gamestate& gamestate = Gamestate::Get();

    U64 friendlyPieces, toSquares, knightSquares;
    int knightSq, toSquare;
    if (gamestate.whiteToMove) {
        knightSquares = gamestate.w_knight;
        friendlyPieces = gamestate.w_pieces;
    } else {
        knightSquares = gamestate.b_knight;
        friendlyPieces = gamestate.b_pieces;
    }

    while (knightSquares) {
        knightSq = popLSB(knightSquares);
        toSquares = MovementTables::knightMoves[knightSq] & ~friendlyPieces & checkMask;
        while (toSquares) {
            toSquare = popLSB(toSquares);
            legalMoves.emplace_back(knightSq, toSquare, isCapture(gamestate, toSquare));
        }
    }
}

void MoveGenerator::GenerateBishopMoves() {
    const Gamestate& gamestate = Gamestate::Get();

    U64 friendlyPieces, sliders;
    U64 northwest, southwest, northeast, southeast;
    U64 most_significant_bit, difference, target_squares = 0;
    int slider;

    if (gamestate.whiteToMove) {
        friendlyPieces = gamestate.w_pieces;
        sliders = gamestate.w_bishop | gamestate.w_queen;
    } else /* black to move */ {
        friendlyPieces = gamestate.b_pieces;
        sliders = gamestate.b_bishop | gamestate.b_queen;
    }

    while (sliders) {
        slider = popLSB(sliders);

        northwest = gamestate.all_pieces & MovementTables::bishopMoves[slider][0];
        southeast = gamestate.all_pieces & MovementTables::bishopMoves[slider][2];
        northeast = gamestate.all_pieces & MovementTables::bishopMoves[slider][1];
        southwest = gamestate.all_pieces & MovementTables::bishopMoves[slider][3];

        most_significant_bit = getMSB(southeast);
        difference = northwest ^ (northwest - most_significant_bit);
        target_squares |= difference & MovementTables::bishopMoves[slider][4] & ~friendlyPieces & checkMask;

        most_significant_bit = getMSB(southwest);
        difference = northeast ^ (northeast - most_significant_bit);
        target_squares |= difference & MovementTables::bishopMoves[slider][5] & ~friendlyPieces & checkMask;

        while (target_squares) {
            int toSquare = popLSB(target_squares);
            legalMoves.emplace_back(slider, toSquare, isCapture(gamestate, toSquare));
        }
    }
}

void MoveGenerator::GenerateRookMoves() {
    const Gamestate& gamestate = Gamestate::Get();

    U64 friendlyPieces, sliders;
    U64 north, south, east, west;
    U64 most_significant_bit, difference, target_squares = 0;
    int slider;

    if (gamestate.whiteToMove) {
        friendlyPieces = gamestate.w_pieces;
        sliders = gamestate.w_rook | gamestate.w_queen;
    } else {
        friendlyPieces = gamestate.b_pieces;
        sliders = gamestate.b_rook | gamestate.b_queen;
    }

    while(sliders) {
        slider = popLSB(sliders);

        north = gamestate.all_pieces & MovementTables::rookMoves[slider][0];
        south = gamestate.all_pieces & MovementTables::rookMoves[slider][2];
        east = gamestate.all_pieces & MovementTables::rookMoves[slider][1];
        west = gamestate.all_pieces & MovementTables::rookMoves[slider][3];

        most_significant_bit = getMSB(south);
        difference = north ^ (north - most_significant_bit);
        target_squares |= difference & MovementTables::rookMoves[slider][4] & ~friendlyPieces & checkMask;

        most_significant_bit = getMSB(west);
        difference = east ^ (east - most_significant_bit);
        target_squares |= difference & MovementTables::rookMoves[slider][5] & ~friendlyPieces & checkMask;

        while (target_squares) {
            int toSquare = popLSB(target_squares);
            legalMoves.emplace_back(slider, toSquare, isCapture(gamestate, toSquare));
        }
    }
}

int MoveGenerator::PerftTree(int depthPly) {
    Gamestate& gamestate = Gamestate::Get();

    if (depthPly == 1) {
        std::vector<Move> legalMoves = GenerateLegalMoves();
        return static_cast<int>(legalMoves.size());
    }

    std::vector<Move> legalMoves = GenerateLegalMoves();
    int nodesFound = 0;
    for (auto move : legalMoves) {
        gamestate.MakeMove(move);
        nodesFound += PerftTree(depthPly - 1);
        gamestate.UndoMove();
    }
    return nodesFound;
}
