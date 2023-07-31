//
// Created by Ryan Baker on 7/1/23.
//

#include "movegen.h"
#include "bitUtils.h"
#include "evaluation.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <chrono>


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
    pinnedPieces = checkMask = enemyAttacks = 0;

    legalMoves.reserve(218);
}

void MoveGenerator::GenerateLegalMoves() {
    legalMoves.clear();

    CalculateEnemyAttacks();
    CalculateCheckMask();
    CalculatePinMasks();

    GenerateKingMoves();

    if (king_is_in_double_check) return;

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
        ++move;
    }
}

void MoveGenerator::CalculateEnemyAttacks() {
    Gamestate& gamestate = Gamestate::Get();

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

    while (enemyKnights) enemyAttacks |= MovementTables::knightMoves[BitUtils::popLSB(enemyKnights)];
    while (diagonalSliders) {
        slider = BitUtils::popLSB(diagonalSliders);

        northwest = allPieces & MovementTables::bishopMoves[slider][0];
        southeast = allPieces & MovementTables::bishopMoves[slider][2];
        northeast = allPieces & MovementTables::bishopMoves[slider][1];
        southwest = allPieces & MovementTables::bishopMoves[slider][3];

        mostSignificantBit = BitUtils::getMSB(southeast);
        difference = northwest ^ (northwest - mostSignificantBit);
        enemyAttacks |= difference & MovementTables::bishopMoves[slider][4];

        mostSignificantBit = BitUtils::getMSB(southwest);
        difference = northeast ^ (northeast - mostSignificantBit);
        enemyAttacks |= difference & MovementTables::bishopMoves[slider][5];
    }
    while (orthogonalSliders) {
        slider = BitUtils::popLSB(orthogonalSliders);

        north = allPieces & MovementTables::rookMoves[slider][0];
        south = allPieces & MovementTables::rookMoves[slider][2];
        east = allPieces & MovementTables::rookMoves[slider][1];
        west = allPieces & MovementTables::rookMoves[slider][3];

        mostSignificantBit = BitUtils::getMSB(south);
        difference = north ^ (north - mostSignificantBit);
        enemyAttacks |= difference & MovementTables::rookMoves[slider][4];

        mostSignificantBit = BitUtils::getMSB(west);
        difference = east ^ (east - mostSignificantBit);
        enemyAttacks |= difference & MovementTables::rookMoves[slider][5];
    }
    enemyAttacks |= MovementTables::kingMoves[BitUtils::getLSB(~friendlyPieces & (*gamestate.bitboards[5] | *gamestate.bitboards[11]))];
}

void MoveGenerator::CalculateCheckMask() {
    Gamestate& gamestate = Gamestate::Get();

    U64 kingPos;
    kingPos = gamestate.whiteToMove ? gamestate.w_king : gamestate.b_king;
    int kingSquare = BitUtils::getLSB(kingPos);
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

        mostSignificantBit = BitUtils::getMSB(southeast);
        difference = northwest ^ (northwest - mostSignificantBit);
        attacks |= difference & MovementTables::bishopMoves[kingSquare][4];

        mostSignificantBit = BitUtils::getMSB(southwest);
        difference = northeast ^ (northeast - mostSignificantBit);
        attacks |= difference & MovementTables::bishopMoves[kingSquare][5];

        if (attacks & diagonalSliders) {
            if (checkMask) {
                king_is_in_double_check = true;
                checkMask = 0;
                return;
            }
            checkMask = BitMasks::segmentMask(kingSquare, BitUtils::getLSB(diagonalSliders & attacks));
        }

        attacks = 0;
        orthogonalSliders = gamestate.b_rook | gamestate.b_queen;
        north = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][0];
        south = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][2];
        east = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][1];
        west = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][3];

        mostSignificantBit = BitUtils::getMSB(south);
        difference = north ^ (north - mostSignificantBit);
        attacks |= difference & MovementTables::rookMoves[kingSquare][4];

        mostSignificantBit = BitUtils::getMSB(west);
        difference = east ^ (east - mostSignificantBit);
        attacks |= difference & MovementTables::rookMoves[kingSquare][5];

        if (attacks & orthogonalSliders) {
            if (checkMask) {
                king_is_in_double_check = true;
                checkMask = 0;
                return;
            }
            king_is_in_double_check = false;
            checkMask = BitMasks::segmentMask(kingSquare, BitUtils::getLSB(orthogonalSliders & attacks));
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

        mostSignificantBit = BitUtils::getMSB(southeast);
        difference = northwest ^ (northwest - mostSignificantBit);
        attacks |= difference & MovementTables::bishopMoves[kingSquare][4];

        mostSignificantBit = BitUtils::getMSB(southwest);
        difference = northeast ^ (northeast - mostSignificantBit);
        attacks |= difference & MovementTables::bishopMoves[kingSquare][5];

        if (attacks & diagonalSliders) {
            if (checkMask) { // if checkMask is not 0 this indicates a double check
                king_is_in_double_check = true;
                checkMask = 0;
                return;
            }
            king_is_in_double_check = false;
            checkMask = BitMasks::segmentMask(kingSquare, BitUtils::getLSB(diagonalSliders & attacks));
        }

        attacks = 0;
        orthogonalSliders = gamestate.w_rook | gamestate.w_queen;
        north = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][0];
        south = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][2];
        east = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][1];
        west = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][3];

        mostSignificantBit = BitUtils::getMSB(south);
        difference = north ^ (north - mostSignificantBit);
        attacks |= difference & MovementTables::rookMoves[kingSquare][4];

        mostSignificantBit = BitUtils::getMSB(west);
        difference = east ^ (east - mostSignificantBit);
        attacks |= difference & MovementTables::rookMoves[kingSquare][5];

        if (attacks & orthogonalSliders) {
            if (checkMask) {
                king_is_in_double_check = true;
                checkMask = 0;
                return;
            }
            king_is_in_double_check = false;
            checkMask = BitMasks::segmentMask(kingSquare, BitUtils::getLSB(orthogonalSliders & attacks));
        }
    }
}

void MoveGenerator::CalculatePinMasks() {
    Gamestate& gamestate = Gamestate::Get();

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
    int kingSquare = BitUtils::getLSB(kingPos), potentialPinnedPiece;

    enemyDiagonalSliders = (gamestate.w_bishop | gamestate.w_queen | gamestate.b_bishop | gamestate.b_queen) & ~friendlyPieces;
    enemyOrthogonalSliders =(gamestate.w_rook | gamestate.w_queen | gamestate.b_rook | gamestate.b_queen) & ~friendlyPieces;
    enPassantRank = gamestate.whiteToMove ? Board::Ranks::rank_5 : Board::Ranks::rank_4;

    northwest = gamestate.all_pieces & MovementTables::bishopMoves[kingSquare][0];
    southeast = gamestate.all_pieces & MovementTables::bishopMoves[kingSquare][2];
    northeast = gamestate.all_pieces & MovementTables::bishopMoves[kingSquare][1];
    southwest = gamestate.all_pieces & MovementTables::bishopMoves[kingSquare][3];

    mostSignificantBit = BitUtils::getMSB(southeast);
    difference = northwest ^ (northwest - mostSignificantBit);
    attacks |= difference & MovementTables::bishopMoves[kingSquare][4];
    mostSignificantBit = BitUtils::getMSB(southwest);
    difference = northeast ^ (northeast - mostSignificantBit);
    attacks |= difference & MovementTables::bishopMoves[kingSquare][5];

    potentialPins = attacks & friendlyPieces;
    while (potentialPins) {
        potentialPinnedPiece = BitUtils::popLSB(potentialPins);
        xRayedSquare = BitMasks::xRay(kingSquare, potentialPinnedPiece, gamestate.all_pieces);
        if (xRayedSquare & enemyDiagonalSliders) {
            pinnedPieces |= 1ULL << potentialPinnedPiece;
            pinMasks[potentialPinnedPiece] = BitMasks::segmentMask(kingSquare, BitUtils::getLSB(xRayedSquare));
        }
    }

    attacks = 0;
    north = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][0];
    south = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][2];
    east = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][1];
    west = gamestate.all_pieces & MovementTables::rookMoves[kingSquare][3];

    mostSignificantBit = BitUtils::getMSB(south);
    difference = north ^ (north - mostSignificantBit);
    attacks |= difference & MovementTables::rookMoves[kingSquare][4];
    mostSignificantBit = BitUtils::getMSB(west);
    difference = east ^ (east - mostSignificantBit);
    attacks |= difference & MovementTables::rookMoves[kingSquare][5];

    potentialPins = attacks & friendlyPieces;
    while (potentialPins) {
        potentialPinnedPiece = BitUtils::popLSB(potentialPins);
        xRayedSquare = BitMasks::xRay(kingSquare, potentialPinnedPiece, gamestate.all_pieces);
        if (xRayedSquare & enemyOrthogonalSliders) {
            pinnedPieces |= 1ULL << potentialPinnedPiece;
            pinMasks[potentialPinnedPiece] = BitMasks::segmentMask(kingSquare, BitUtils::getLSB(xRayedSquare));
        }
    }

    if (!gamestate.moveLog.empty()) {
        if (gamestate.moveLog.top().moveFlag & MoveFlags::doublePawnPush &&
            kingPos & enPassantRank &&
            friendlyPieces & (gamestate.w_pawn | gamestate.b_pawn) & enPassantRank &&
            enemyOrthogonalSliders & enPassantRank) {
            for (int potentialPinnedEnPassant : BitUtils::getBits(attacks & (gamestate.w_pawn | gamestate.b_pawn) & (east | west))) {
                xRayedSquare = BitMasks::xRay(kingSquare, potentialPinnedEnPassant, gamestate.all_pieces);
                U64 doubleXRayedSquare = BitMasks::xRay(potentialPinnedEnPassant, BitUtils::getLSB(xRayedSquare), gamestate.all_pieces);
                if ((gamestate.mailbox[BitUtils::getLSB(xRayedSquare)] ^ gamestate.mailbox[potentialPinnedEnPassant]) & 8 &&
                    std::abs(BitUtils::getLSB(xRayedSquare) % 8 - potentialPinnedEnPassant % 8) == 1 &&
                    doubleXRayedSquare & enemyOrthogonalSliders)  {
                    if (gamestate.whiteToMove) {
                        if (gamestate.mailbox[potentialPinnedEnPassant] & 8) {
                            pinnedPieces |= xRayedSquare;
                            pinMasks[BitUtils::getLSB(xRayedSquare)] = ~((1ULL << potentialPinnedEnPassant) << 8);
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
                            pinMasks[BitUtils::getLSB(xRayedSquare)] = ~((1ULL << potentialPinnedEnPassant) >> 8);
                        }
                    }
                }
            }
        }
    }
}

void MoveGenerator::GenerateKingMoves() {
    Gamestate& gamestate = Gamestate::Get();

    int king_sq;
    U64 friendly_pieces, to_squares;
    if (gamestate.whiteToMove) {
        king_sq = BitUtils::getLSB(gamestate.w_king);
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
        king_sq = BitUtils::getLSB(gamestate.b_king);
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
        int toSquare = BitUtils::popLSB(to_squares);
        legalMoves.emplace_back(king_sq, toSquare, isCapture(gamestate, toSquare));
    }
}

void MoveGenerator::GeneratePawnMoves() {
    Gamestate& gamestate = Gamestate::Get();

    U64 pawnSquares, singleStep, doubleStep, leftCapt, rightCapt, promotion, promotionLeftCapt, promotionRightCapt;
    U64 singleStep_f, doubleStep_f, leftCapt_f, rightCapt_f, promotion_f, promotionLeftCapt_f, promotionRightCapt_f;
    U64 promotionRank, enPassantRank, oneStepRank;
    U64 enemyPieces, enPassant_f = 0ULL, enPassantSquare = 0ULL, enPassantCheckMask = 0ULL;
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

    while (singleStep) legalMoves.emplace_back(BitUtils::popLSB(singleStep_f), BitUtils::popLSB(singleStep), MoveFlags::quietMove);
    while (doubleStep) legalMoves.emplace_back(BitUtils::popLSB(doubleStep_f), BitUtils::popLSB(doubleStep), MoveFlags::doublePawnPush);
    while (leftCapt) legalMoves.emplace_back(BitUtils::popLSB(leftCapt_f), BitUtils::popLSB(leftCapt), MoveFlags::capture);
    while (rightCapt) legalMoves.emplace_back(BitUtils::popLSB(rightCapt_f), BitUtils::popLSB(rightCapt), MoveFlags::capture);
    while (enPassant_f) legalMoves.emplace_back(BitUtils::popLSB(enPassant_f), BitUtils::getLSB(enPassantSquare), MoveFlags::enPassant);
    while (promotion) {
        fromSq = BitUtils::popLSB(promotion_f);
        toSq = BitUtils::popLSB(promotion);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::knightPromotion);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::bishopPromotion);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::rookPromotion);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::queenPromotion);
    }
    while (promotionLeftCapt) {
        fromSq = BitUtils::popLSB(promotionLeftCapt_f);
        toSq = BitUtils::popLSB(promotionLeftCapt);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::knightPromoCapt);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::bishopPromoCapt);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::rookPromoCapt);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::queenPromoCapt);
    }
    while (promotionRightCapt) {
        fromSq = BitUtils::popLSB(promotionRightCapt_f);
        toSq = BitUtils::popLSB(promotionRightCapt);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::knightPromoCapt);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::bishopPromoCapt);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::rookPromoCapt);
        legalMoves.emplace_back(fromSq, toSq, MoveFlags::queenPromoCapt);
    }
}

void MoveGenerator::GenerateKnightMoves() {
    Gamestate& gamestate = Gamestate::Get();

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
        knightSq = BitUtils::popLSB(knightSquares);
        toSquares = MovementTables::knightMoves[knightSq] & ~friendlyPieces & checkMask;
        while (toSquares) {
            toSquare = BitUtils::popLSB(toSquares);
            legalMoves.emplace_back(knightSq, toSquare, isCapture(gamestate, toSquare));
        }
    }
}

void MoveGenerator::GenerateBishopMoves() {
    Gamestate& gamestate = Gamestate::Get();

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
        slider = BitUtils::popLSB(sliders);

        northwest = gamestate.all_pieces & MovementTables::bishopMoves[slider][0];
        southeast = gamestate.all_pieces & MovementTables::bishopMoves[slider][2];
        northeast = gamestate.all_pieces & MovementTables::bishopMoves[slider][1];
        southwest = gamestate.all_pieces & MovementTables::bishopMoves[slider][3];

        most_significant_bit = BitUtils::getMSB(southeast);
        difference = northwest ^ (northwest - most_significant_bit);
        target_squares |= difference & MovementTables::bishopMoves[slider][4] & ~friendlyPieces & checkMask;

        most_significant_bit = BitUtils::getMSB(southwest);
        difference = northeast ^ (northeast - most_significant_bit);
        target_squares |= difference & MovementTables::bishopMoves[slider][5] & ~friendlyPieces & checkMask;

        while (target_squares) {
            int toSquare = BitUtils::popLSB(target_squares);
            legalMoves.emplace_back(slider, toSquare, isCapture(gamestate, toSquare));
        }
    }
}

void MoveGenerator::GenerateRookMoves() {
    Gamestate& gamestate = Gamestate::Get();

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
        slider = BitUtils::popLSB(sliders);

        north = gamestate.all_pieces & MovementTables::rookMoves[slider][0];
        south = gamestate.all_pieces & MovementTables::rookMoves[slider][2];
        east = gamestate.all_pieces & MovementTables::rookMoves[slider][1];
        west = gamestate.all_pieces & MovementTables::rookMoves[slider][3];

        most_significant_bit = BitUtils::getMSB(south);
        difference = north ^ (north - most_significant_bit);
        target_squares |= difference & MovementTables::rookMoves[slider][4] & ~friendlyPieces & checkMask;

        most_significant_bit = BitUtils::getMSB(west);
        difference = east ^ (east - most_significant_bit);
        target_squares |= difference & MovementTables::rookMoves[slider][5] & ~friendlyPieces & checkMask;

        while (target_squares) {
            int toSquare = BitUtils::popLSB(target_squares);
            legalMoves.emplace_back(slider, toSquare, isCapture(gamestate, toSquare));
        }
    }
}

int MoveGenerator::PerftTree(int depthPly) {
    Gamestate& gamestate = Gamestate::Get();

    if (depthPly == 1) {
        GenerateLegalMoves();
        return static_cast<int>(legalMoves.size());
    }

    GenerateLegalMoves();
    int nodesFound = 0;
    std::vector<Move> currentLegalMoves = legalMoves;
    for (auto move : currentLegalMoves) {
        gamestate.MakeMove(move);
        nodesFound += PerftTree(depthPly - 1);
        gamestate.UndoMove();
    }
    return nodesFound;
}

void MoveGenerator::PerftTest() {
    Gamestate& gamestate = Gamestate::Get();

    float totalTime = 0, averageNPS;

    gamestate.Seed("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    auto start = std::chrono::high_resolution_clock::now();
    int position1nodes = PerftTree(5);
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
    if (position1nodes != 4865609) {
        std::cout << "Test 1 failed  -  4,865,609 nodes expected  -  " << position1nodes << " nodes found" << std::endl;
    } else {
        std::cout << "Test 1 passed in " << (duration.count() / pow(10, 9)) << " seconds  -  " <<
                  4865609 / (duration.count() / pow(10, 9)) << " nodes per second" << std::endl;
    }
    totalTime += (duration.count() / pow(10, 9));


    gamestate.Seed("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    start = std::chrono::high_resolution_clock::now();
    int position2nodes = PerftTree(5);
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
    if (position2nodes != 193690690) {
        std::cout << "Test 2 failed  -  193,690,690 nodes expected  -  " << position2nodes << " nodes found" << std::endl;
    } else {
        std::cout << "Test 2 passed in " << (duration.count() / pow(10, 9)) << " seconds  -  " <<
                  193690690 / (duration.count() / pow(10, 9)) << " nodes per second" << std::endl;
    }
    totalTime += (duration.count() / pow(10, 9));


    gamestate.Seed("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");
    start = std::chrono::high_resolution_clock::now();
    int position3nodes = PerftTree(5);
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
    if (position3nodes != 674624) {
        std::cout << "Test 3 failed  -  674,624 nodes expected  -  " << position3nodes << " nodes found" << std::endl;
    } else {
        std::cout << "Test 3 passed in " << (duration.count() / pow(10, 9)) << " seconds  -  " <<
                  674624 / (duration.count() / pow(10, 9)) << " nodes per second" << std::endl;
    }
    totalTime += (duration.count() / pow(10, 9));


    gamestate.Seed("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    start = std::chrono::high_resolution_clock::now();
    int position4nodes = PerftTree(5);
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
    if (position4nodes != 15833292) {
        std::cout << "Test 4 failed  -  15,833,292 nodes expected  -  " << position4nodes << " nodes found" << std::endl;
    } else {
        std::cout << "Test 4 passed in " << (duration.count() / pow(10, 9)) << " seconds  -  " <<
                  15833292 / (duration.count() / pow(10, 9)) << " nodes per second" << std::endl;
    }
    totalTime += (duration.count() / pow(10, 9));


    gamestate.Seed("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    start = std::chrono::high_resolution_clock::now();
    int position5nodes = PerftTree(5);
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
    if (position5nodes != 89941194) {
        std::cout << "Test 5 failed  -  89,941,194 nodes expected  -  " << position5nodes << " nodes found" << std::endl;
    } else {
        std::cout << "Test 5 passed in " << (duration.count() / pow(10, 9)) << " seconds  -  " <<
                  89941194 / (duration.count() / pow(10, 9)) << " nodes per second" << std::endl;
    }
    totalTime += (duration.count() / pow(10, 9));


    gamestate.Seed("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    start = std::chrono::high_resolution_clock::now();
    int position6nodes = PerftTree(5);
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
    if (position6nodes != 164075551) {
        std::cout << "Test 6 failed  -  164,075,551 nodes expected  -  " << position6nodes << " nodes found" << std::endl;
    } else {
        std::cout << "Test 6 passed in " << (duration.count() / pow(10, 9)) << " seconds  -  " <<
                  164075551 / (duration.count() / pow(10, 9)) << " nodes per second" << std::endl;
    }
    totalTime += (duration.count() / pow(10, 9));
    averageNPS = float(position1nodes + position2nodes + position3nodes + position4nodes + position5nodes + position6nodes) / totalTime;

    std::cout << "Total time: " << totalTime << " seconds" << std::endl;
    std::cout << "Average NPS: " << averageNPS << std::endl << std::endl;
}
