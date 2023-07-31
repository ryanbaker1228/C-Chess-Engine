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


MoveGenerator* MoveGenerator::instance = nullptr;
/*
int createMoveTree(GAMESTATE& gamestate, int depth_ply) {
    if (depth_ply == 1) {
        return static_cast<int>(MoveGenerator::GenerateLegalMoves(gamestate).size());
    }
    int num_pos = 0;
    for (struct Move move : MoveGenerator::GenerateLegalMoves(gamestate)) {
        gamestate.makeMove(move);
        num_pos += createMoveTree(gamestate, depth_ply - 1);
        gamestate.undoMove();
    }
    return num_pos;
}

int MoveSearch(GAMESTATE* gamestate, int depth_ply, int alpha, int beta) {
    if (depth_ply == 0) {
        return StaticEvaluate(gamestate);
    }

    std::vector<struct Move> legal_moves = MoveGenerator::GenerateLegalMoves(*gamestate);

    if (gamestate->whiteToMove) {
        int maxEval = BLACK_WIN;
        for (Move move : legal_moves) {
            gamestate->makeMove(move);
            int evaluation = MoveSearch(gamestate, depth_ply - 1, alpha, beta);
            gamestate->undoMove();
            maxEval = std::max(maxEval, evaluation);
            alpha = std::max(alpha, evaluation);
            if (beta <= alpha) {
                break;
            }
        }
        return maxEval;
    } else {
        int minEval = WHITE_WIN;
        for (Move move : legal_moves) {
            gamestate->makeMove(move);
            int evaluation = MoveSearch(gamestate, depth_ply - 1, alpha, beta);
            gamestate->undoMove();
            minEval = std::min(minEval, evaluation);
            beta = std::min(beta, evaluation);
            if (beta <= alpha) {
                break;
            }
        }
        return minEval;
    }
}

std::vector<Move> MoveGenerator::GenerateLegalMoves(GAMESTATE& gamestate) {

    for (Move move : gamestate.move_log) {
        if (move.startSquare == Board::Squares::a1) gamestate.legality &= ~legalityBits::whiteLongCastleMask;
        if (move.startSquare == Board::Squares::h1) gamestate.legality &= ~legalityBits::whiteShortCastleMask;
        if (move.startSquare == Board::Squares::a8) gamestate.legality &= ~legalityBits::blackLongCastleMask;
        if (move.startSquare == Board::Squares::h8) gamestate.legality &= ~legalityBits::blackShortCastleMask;
    }

    std::vector<Move> pseudoLegalMoves, legalMoves;
    pseudoLegalMoves.reserve(218);

    U64 friendlyPieces, kingPos;
    if (gamestate.whiteToMove) {
        friendlyPieces = gamestate.w_pieces;
        kingPos = gamestate.w_king;
    } else {
        friendlyPieces = gamestate.b_pieces;
        kingPos = gamestate.b_king;
    }

    U64 attackedSquares = FindAttackedSquares(gamestate, friendlyPieces, kingPos);
    U64 checkMask = (attackedSquares & kingPos) ? CreateCheckMask(gamestate, kingPos) : ~0ULL;
    FindPinnedPieces(gamestate);

    GeneratePawnMoves(gamestate, pseudoLegalMoves, checkMask);
    GenerateKnightMoves(gamestate, pseudoLegalMoves, checkMask);
    GenerateDiagonalSliderMoves(gamestate, pseudoLegalMoves, checkMask);
    GenerateOrthogonalSliderMoves(gamestate, pseudoLegalMoves, checkMask);
    GenerateKingMoves(gamestate, pseudoLegalMoves, attackedSquares);

    legalMoves.reserve(pseudoLegalMoves.size());

    for (Move move : pseudoLegalMoves) {
        if (1ULL << move.startSquare & gamestate.pinned_pieces) {
            if (!(1ULL << move.endSquare & gamestate.pin_masks[move.startSquare])) {
                continue;
            }
        }
        legalMoves.emplace_back(move);
    }
    //std::sort(legalMoves.begin(), legalMoves.end(), [](Move move1, Move move2) {
    //    return (move1.promise > move2.promise) * !(move1.moving_piece & 0b1000) +
    //           (move1.promise < move2.promise) * (move1.moving_piece & 0b1000);
    //});

    return legalMoves;
}

void MoveGenerator::GeneratePawnMoves(const GAMESTATE& gamestate, std::vector<Move>& legalMoves, U64 checkMask) {
    U64 enemy_pieces, pawn_sqs, one_step, two_step, left_capt, right_capt, leftEnPassant = 0, leftEnPassantFrom, rightEnPassantFrom,
            one_step_from, two_step_from, left_capt_from, right_capt_from, rightEnPassant = 0, enPassantSquare = 0,
            promotion, promotion_from, promotion_capt_left, promotion_capt_left_from, promotion_capt_right, promotion_capt_right_from,
            checkMaskEnPassant = 0;
    if (gamestate.whiteToMove) {
        pawn_sqs = gamestate.w_pawn;
        enemy_pieces = gamestate.b_pieces;
        if (!gamestate.move_log.empty()) {
            if (gamestate.move_log.back().moveFlag == MoveFlags::doublePawnPush) {
                if (gamestate.move_log.back().moveFlag == MoveFlags::doublePawnPush) {
                    enPassantSquare = (1ULL << gamestate.move_log.back().endSquare) << 8;
                    if ((enPassantSquare >> 8) & checkMask) checkMaskEnPassant = enPassantSquare;
                    leftEnPassant =
                            (pawn_sqs & ~Board::Files::aFile) << 7 & enPassantSquare & (checkMask | checkMaskEnPassant);
                    rightEnPassant =
                            (pawn_sqs & ~Board::Files::hFile) << 9 & enPassantSquare & (checkMask | checkMaskEnPassant);
                    leftEnPassantFrom = leftEnPassant >> 7;
                    rightEnPassantFrom = rightEnPassant >> 9;
                }
            }
        }

        one_step = (pawn_sqs & ~Board::Ranks::rank_7) << 8 & gamestate.empty_sqs & checkMask;
        two_step = (pawn_sqs & (Board::Ranks::rank_3 & gamestate.empty_sqs) >> 8) << 16 & gamestate.empty_sqs &
                   checkMask;
        left_capt = (pawn_sqs & ~Board::Files::aFile & ~Board::Ranks::rank_7) << 7 & enemy_pieces & checkMask;
        right_capt = (pawn_sqs & ~Board::Files::hFile & ~Board::Ranks::rank_7) << 9 & enemy_pieces & checkMask;
        promotion = (pawn_sqs & Board::Ranks::rank_7) << 8 & gamestate.empty_sqs & checkMask;
        promotion_capt_left =
                (pawn_sqs & ~Board::Files::aFile & Board::Ranks::rank_7) << 7 & enemy_pieces & checkMask;
        promotion_capt_right =
                (pawn_sqs & ~Board::Files::hFile & Board::Ranks::rank_7) << 9 & enemy_pieces & checkMask;

        one_step_from = one_step >> 8;
        two_step_from = two_step >> 16;
        left_capt_from = left_capt >> 7;
        right_capt_from = right_capt >> 9;
        promotion_from = promotion >> 8;
        promotion_capt_left_from = promotion_capt_left >> 7;
        promotion_capt_right_from = promotion_capt_right >> 9;

    } else {
        pawn_sqs = gamestate.b_pawn;
        enemy_pieces = gamestate.w_pieces;
        if (!gamestate.move_log.empty()) {
            if (gamestate.move_log.back().moveFlag == MoveFlags::doublePawnPush) {
                if (gamestate.move_log.back().moveFlag == MoveFlags::doublePawnPush) {
                    enPassantSquare = (1ULL << gamestate.move_log.back().endSquare) >> 8;
                    if ((enPassantSquare << 8) & checkMask) checkMaskEnPassant = enPassantSquare;
                    leftEnPassant |= (pawn_sqs & ~Board::Files::hFile) >> 7 & (enPassantSquare) &
                                     (checkMask | checkMaskEnPassant);
                    rightEnPassant |= (pawn_sqs & ~Board::Files::aFile) >> 9 & (enPassantSquare) &
                                      (checkMask | checkMaskEnPassant);
                    leftEnPassantFrom = leftEnPassant << 7;
                    rightEnPassantFrom = rightEnPassant << 9;
                }
            }
        }
        one_step = (pawn_sqs & ~Board::Ranks::rank_2) >> 8 & gamestate.empty_sqs & checkMask;
        two_step = (pawn_sqs & (Board::Ranks::rank_6 & gamestate.empty_sqs) << 8) >> 16 & gamestate.empty_sqs &
                   checkMask;
        left_capt = (pawn_sqs & ~Board::Files::hFile & ~Board::Ranks::rank_2) >> 7 & enemy_pieces & checkMask;
        right_capt = (pawn_sqs & ~Board::Files::aFile & ~Board::Ranks::rank_2) >> 9 & enemy_pieces & checkMask;
        promotion = (pawn_sqs & Board::Ranks::rank_2) >> 8 & gamestate.empty_sqs & checkMask;
        promotion_capt_left =
                (pawn_sqs & ~Board::Files::hFile & Board::Ranks::rank_2) >> 7 & enemy_pieces & checkMask;
        promotion_capt_right =
                (pawn_sqs & ~Board::Files::aFile & Board::Ranks::rank_2) >> 9 & enemy_pieces & checkMask;

        one_step_from = one_step << 8;
        two_step_from = two_step << 16;
        left_capt_from = left_capt << 7;
        right_capt_from = right_capt << 9;
        promotion_from = promotion << 8;
        promotion_capt_left_from = promotion_capt_left << 7;
        promotion_capt_right_from = promotion_capt_right << 9;
    }

    while (one_step) legalMoves.emplace_back(BitUtils::popLSB(one_step_from), BitUtils::popLSB(one_step));
    while (two_step) legalMoves.emplace_back(BitUtils::popLSB(two_step_from), BitUtils::popLSB(two_step), MoveFlags::doublePawnPush);
    while (left_capt) legalMoves.emplace_back(BitUtils::popLSB(left_capt_from), BitUtils::popLSB(left_capt), MoveFlags::capture);
    while (right_capt) legalMoves.emplace_back(BitUtils::popLSB(right_capt_from), BitUtils::popLSB(right_capt), MoveFlags::capture);
    while (leftEnPassant) legalMoves.emplace_back(BitUtils::popLSB(leftEnPassantFrom), BitUtils::popLSB(leftEnPassant), MoveFlags::enPassant);
    while (rightEnPassant) legalMoves.emplace_back(BitUtils::popLSB(rightEnPassantFrom), BitUtils::popLSB(rightEnPassant), MoveFlags::enPassant);
    while (promotion) {
        int fromSquare = BitUtils::popLSB(promotion_from);
        int toSquare = BitUtils::popLSB(promotion);
        legalMoves.emplace_back(fromSquare, toSquare, MoveFlags::knightPromotion);
        legalMoves.emplace_back(fromSquare, toSquare, MoveFlags::bishopPromotion);
        legalMoves.emplace_back(fromSquare, toSquare, MoveFlags::rookPromotion);
        legalMoves.emplace_back(fromSquare, toSquare, MoveFlags::queenPromotion);
    }
    while (promotion_capt_left) {
        int fromSquare = BitUtils::popLSB(promotion_capt_left_from);
        int toSquare = BitUtils::popLSB(promotion_capt_left);
        legalMoves.emplace_back(fromSquare, toSquare, MoveFlags::knightPromoCapt);
        legalMoves.emplace_back(fromSquare, toSquare, MoveFlags::bishopPromoCapt);
        legalMoves.emplace_back(fromSquare, toSquare, MoveFlags::rookPromoCapt);
        legalMoves.emplace_back(fromSquare, toSquare, MoveFlags::queenPromoCapt);
    }
    while (promotion_capt_right) {
        int fromSquare = BitUtils::popLSB(promotion_capt_right_from);
        int toSquare = BitUtils::popLSB(promotion_capt_right);
        legalMoves.emplace_back(fromSquare, toSquare, MoveFlags::knightPromoCapt);
        legalMoves.emplace_back(fromSquare, toSquare, MoveFlags::bishopPromoCapt);
        legalMoves.emplace_back(fromSquare, toSquare, MoveFlags::rookPromoCapt);
        legalMoves.emplace_back(fromSquare, toSquare, MoveFlags::queenPromoCapt);
    }
}

void MoveGenerator::GenerateKnightMoves(const GAMESTATE& gamestate, std::vector<Move>& legalMoves, U64 checkMask) {
    U64 friendly_pieces, to_squares, knight_sqs;
    int knightSq;
    if (gamestate.whiteToMove) {
        knight_sqs = gamestate.w_knight;
        friendly_pieces = gamestate.w_pieces;
    } else {
        knight_sqs = gamestate.b_knight;
        friendly_pieces = gamestate.b_pieces;
    }

    while (knight_sqs) {
        knightSq = BitUtils::popLSB(knight_sqs);
        to_squares = MovementTables::knightMoves[knightSq] & ~friendly_pieces & checkMask;
        while (to_squares) {
            int toSquare = BitUtils::popLSB(to_squares);
            legalMoves.emplace_back(knightSq, toSquare, isCapture(gamestate, toSquare));
        }
    }
}

void MoveGenerator::GenerateDiagonalSliderMoves(const GAMESTATE& gamestate, std::vector<Move>& legalMoves, U64 checkMask) {
    U64 friendly_pieces, sliders, northwest, southwest, northeast, southeast,
            most_significant_bit, difference, target_squares = 0;
    int slider;

    if (gamestate.whiteToMove) {
        friendly_pieces = gamestate.w_pieces;
        sliders = gamestate.w_bishop | gamestate.w_queen;
    } else {
        friendly_pieces = gamestate.b_pieces;
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
        target_squares |= difference & MovementTables::bishopMoves[slider][4] & ~friendly_pieces & checkMask;

        most_significant_bit = BitUtils::getMSB(southwest);
        difference = northeast ^ (northeast - most_significant_bit);
        target_squares |= difference & MovementTables::bishopMoves[slider][5] & ~friendly_pieces & checkMask;

        while (target_squares) {
            int toSquare = BitUtils::popLSB(target_squares);
            legalMoves.emplace_back(slider, toSquare, isCapture(gamestate, toSquare));
        }
    }
}

void MoveGenerator::GenerateOrthogonalSliderMoves(const GAMESTATE& gamestate, std::vector<Move>& legalMoves, U64 checkMask) {
    U64 friendly_pieces, sliders, north, south, east, west, most_significant_bit, difference, target_squares = 0;
    int slider;

    if (gamestate.whiteToMove) {
        friendly_pieces = gamestate.w_pieces;
        sliders = gamestate.w_rook | gamestate.w_queen;
    } else {
        friendly_pieces = gamestate.b_pieces;
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
        target_squares |= difference & MovementTables::rookMoves[slider][4] & ~friendly_pieces & checkMask;

        most_significant_bit = BitUtils::getMSB(west);
        difference = east ^ (east - most_significant_bit);
        target_squares |= difference & MovementTables::rookMoves[slider][5] & ~friendly_pieces & checkMask;

        while (target_squares) {
            int toSquare = BitUtils::popLSB(target_squares);
            legalMoves.emplace_back(slider, toSquare, isCapture(gamestate, toSquare));
        }
    }
}

void MoveGenerator::GenerateKingMoves(const GAMESTATE& gamestate, std::vector<Move>& legalMoves, U64 enemyAttacks) {
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

U64 MoveGenerator::CreateCheckMask(const GAMESTATE& gamestate, U64 kingPos)  {
    int kingSquare = BitUtils::getLSB(kingPos);

    U64 checkMask, orthogonalSliders, diagonalSliders, mostSignificantBit, difference, attacks = 0,
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
            if (checkMask) { // if checkMask is not 0 this indicates a double check
                return 0ULL;
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
                return 0ULL;
            }
            return BitMasks::segmentMask(kingSquare, BitUtils::getLSB(orthogonalSliders & attacks));
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
                return 0ULL;
            }
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
                return 0ULL;
            }
            return BitMasks::segmentMask(kingSquare, BitUtils::getLSB(orthogonalSliders & attacks));
        }
    }
    return checkMask;
}

U64 MoveGenerator::FindAttackedSquares(const GAMESTATE& gamestate, U64 friendlyPieces, U64 kingPos) {
    U64 attacks, orthogonalSliders, diagonalSliders, enemyKnights, allPieces,
        north, south, east, west, northeast, northwest, southeast, southwest, mostSignificantBit, difference;
    int slider;

    if (gamestate.whiteToMove) {
        attacks = (*gamestate.bitboards[6] & ~Board::Files::hFile) >> 7 | (*gamestate.bitboards[6] & ~Board::Files::aFile) >> 9;
        friendlyPieces = gamestate.w_pieces;
        kingPos = gamestate.w_king;
    } else {
        attacks = (*gamestate.bitboards[0] & ~Board::Files::aFile) << 7 | (*gamestate.bitboards[0] & ~Board::Files::hFile) << 9;
        friendlyPieces = gamestate.b_pieces;
        kingPos = gamestate.b_king;
    }

    allPieces = gamestate.all_pieces ^ kingPos;
    enemyKnights = (*gamestate.bitboards[1] | *gamestate.bitboards[7]) & ~friendlyPieces;
    diagonalSliders = (*gamestate.bitboards[2] | *gamestate.bitboards[4] | *gamestate.bitboards[8] | *gamestate.bitboards[10]) & ~friendlyPieces;
    orthogonalSliders =(*gamestate.bitboards[3] | *gamestate.bitboards[4] | *gamestate.bitboards[9] | *gamestate.bitboards[10]) & ~friendlyPieces;

    while (enemyKnights) attacks |= MovementTables::knightMoves[BitUtils::popLSB(enemyKnights)];
    while (diagonalSliders) {
        slider = BitUtils::popLSB(diagonalSliders);

        northwest = allPieces & MovementTables::bishopMoves[slider][0];
        southeast = allPieces & MovementTables::bishopMoves[slider][2];
        northeast = allPieces & MovementTables::bishopMoves[slider][1];
        southwest = allPieces & MovementTables::bishopMoves[slider][3];

        mostSignificantBit = BitUtils::getMSB(southeast);
        difference = northwest ^ (northwest - mostSignificantBit);
        attacks |= difference & MovementTables::bishopMoves[slider][4];

        mostSignificantBit = BitUtils::getMSB(southwest);
        difference = northeast ^ (northeast - mostSignificantBit);
        attacks |= difference & MovementTables::bishopMoves[slider][5];
    }
    while (orthogonalSliders) {
        slider = BitUtils::popLSB(orthogonalSliders);

        north = allPieces & MovementTables::rookMoves[slider][0];
        south = allPieces & MovementTables::rookMoves[slider][2];
        east = allPieces & MovementTables::rookMoves[slider][1];
        west = allPieces & MovementTables::rookMoves[slider][3];

        mostSignificantBit = BitUtils::getMSB(south);
        difference = north ^ (north - mostSignificantBit);
        attacks |= difference & MovementTables::rookMoves[slider][4];

        mostSignificantBit = BitUtils::getMSB(west);
        difference = east ^ (east - mostSignificantBit);
        attacks |= difference & MovementTables::rookMoves[slider][5];
    }
    attacks |= MovementTables::kingMoves[BitUtils::getLSB(~friendlyPieces & (*gamestate.bitboards[5] | *gamestate.bitboards[11]))];

    return attacks;
}

void MoveGenerator::FindPinnedPieces(GAMESTATE& gamestate) {
    gamestate.pinned_pieces = 0;

    U64 kingPos = (gamestate.whiteToMove) ? gamestate.w_king : gamestate.b_king;
    int kingSquare = BitUtils::getLSB(kingPos), potentialPinnedPiece;
    U64 friendlyPieces, enemyDiagonalSliders, enemyOrthogonalSliders, mostSignificantBit, difference, attacks = 0,
        north, south, east, west, northeast, northwest, southeast, southwest, xRayedSquare, enPassantRank,
        potentialPins;

    friendlyPieces = *gamestate.armies[gamestate.whiteToMove];
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
            gamestate.pinned_pieces |= 1ULL << potentialPinnedPiece;
            gamestate.pin_masks[potentialPinnedPiece] = BitMasks::segmentMask(kingSquare, BitUtils::getLSB(xRayedSquare));
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
            gamestate.pinned_pieces |= 1ULL << potentialPinnedPiece;
            gamestate.pin_masks[potentialPinnedPiece] = BitMasks::segmentMask(kingSquare, BitUtils::getLSB(xRayedSquare));
        }
    }

    if (!gamestate.move_log.empty()) {
        if (gamestate.move_log.back().moveFlag & MoveFlags::doublePawnPush &&
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
                            gamestate.pinned_pieces |= xRayedSquare;
                            gamestate.pin_masks[BitUtils::getLSB(xRayedSquare)] = ~((1ULL << potentialPinnedEnPassant) << 8);
                        } else {
                            gamestate.pinned_pieces |= 1ULL << potentialPinnedEnPassant;
                            gamestate.pin_masks[potentialPinnedEnPassant] = ~(xRayedSquare << 8);
                        }
                    } else {
                        if (gamestate.mailbox[potentialPinnedEnPassant] & 8) {
                            gamestate.pinned_pieces |= 1ULL << potentialPinnedEnPassant;
                            gamestate.pin_masks[potentialPinnedEnPassant] = ~(xRayedSquare >> 8);
                        } else {
                            gamestate.pinned_pieces |= xRayedSquare;
                            gamestate.pin_masks[BitUtils::getLSB(xRayedSquare)] = ~((1ULL << potentialPinnedEnPassant) >> 8);
                        }
                    }
                }
            }
        }
    }
}

void MoveGenerator::TestMoveGenerator() {
    GAMESTATE position1 = GAMESTATE("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    GAMESTATE position2 = GAMESTATE("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    GAMESTATE position3 = GAMESTATE("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");
    GAMESTATE position4 = GAMESTATE("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    GAMESTATE position5 = GAMESTATE("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    GAMESTATE position6 = GAMESTATE("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    float totalTime = 0, averageNPS;

    auto start = std::chrono::high_resolution_clock::now();
    int position1nodes = createMoveTree(position1, 5);
    auto stop = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
    if (position1nodes != 4865609) {
        std::cout << "Test 1 failed  -  4,865,609 nodes expected  -  " << position1nodes << " nodes found" << std::endl;
    } else {
        std::cout << "Test 1 passed in " << (duration.count() / pow(10, 9)) << " seconds  -  " <<
        4865609 / (duration.count() / pow(10, 9)) << " nodes per second" << std::endl;
    }
    totalTime += (duration.count() / pow(10, 9));

    start = std::chrono::high_resolution_clock::now();
    int position2nodes = createMoveTree(position2, 5);
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
    if (position2nodes != 193690690) {
        std::cout << "Test 2 failed  -  193,690,690 nodes expected  -  " << position2nodes << " nodes found" << std::endl;
    } else {
        std::cout << "Test 2 passed in " << (duration.count() / pow(10, 9)) << " seconds  -  " <<
        193690690 / (duration.count() / pow(10, 9)) << " nodes per second" << std::endl;
    }
    totalTime += (duration.count() / pow(10, 9));

    start = std::chrono::high_resolution_clock::now();
    int position3nodes = createMoveTree(position3, 5);
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
    if (position3nodes != 674624) {
        std::cout << "Test 3 failed  -  674,624 nodes expected  -  " << position3nodes << " nodes found" << std::endl;
    } else {
        std::cout << "Test 3 passed in " << (duration.count() / pow(10, 9)) << " seconds  -  " <<
        674624 / (duration.count() / pow(10, 9)) << " nodes per second" << std::endl;
    }
    totalTime += (duration.count() / pow(10, 9));

    start = std::chrono::high_resolution_clock::now();
    int position4nodes = createMoveTree(position4, 5);
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
    if (position4nodes != 15833292) {
        std::cout << "Test 4 failed  -  15,833,292 nodes expected  -  " << position4nodes << " nodes found" << std::endl;
    } else {
        std::cout << "Test 4 passed in " << (duration.count() / pow(10, 9)) << " seconds  -  " <<
                     15833292 / (duration.count() / pow(10, 9)) << " nodes per second" << std::endl;
    }
    totalTime += (duration.count() / pow(10, 9));

    start = std::chrono::high_resolution_clock::now();
    int position5nodes = createMoveTree(position5, 5);
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
    if (position5nodes != 89941194) {
        std::cout << "Test 5 failed  -  89,941,194 nodes expected  -  " << position5nodes << " nodes found" << std::endl;
    } else {
        std::cout << "Test 5 passed in " << (duration.count() / pow(10, 9)) << " seconds  -  " <<
        89941194 / (duration.count() / pow(10, 9)) << " nodes per second" << std::endl;
    }
    totalTime += (duration.count() / pow(10, 9));

    start = std::chrono::high_resolution_clock::now();
    int position6nodes = createMoveTree(position6, 5);
    stop = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
    if (position6nodes != 164075551) {
        std::cout << "Test 6 failed  -  164,075,551 nodes expected  -  " << position6nodes << " nodes found" << std::endl;
    } else {
        std::cout << "Test 6 passed in " << (duration.count() / pow(10, 9)) << " seconds  -  " <<
        164075551 / (duration.count() / pow(10, 9)) << " nodes per second" << std::endl;
    }
    totalTime += (duration.count() / pow(10, 9));
    averageNPS = float(4865609 + 193690690 + 674624 + 15833292 + 89941194 + 164075551) / totalTime;

    std::cout << "Total time: " << totalTime << " seconds" << std::endl;
    std::cout << "Average NPS: " << averageNPS << std::endl << std::endl;
}
*/
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

///*
MoveGenerator::MoveGenerator() {
    pinnedPieces = checkMask = enemyAttacks = 0;

    legalMoves.reserve(218);
}

void MoveGenerator::Seed(GAMESTATE& position) {
    gamestate = &position;
    legalMoves.clear();
}

void MoveGenerator::GenerateLegalMoves(GenerationType type) {
    legalMoves.clear();
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
    U64 friendlyPieces, kingPos, orthogonalSliders, diagonalSliders, enemyKnights, allPieces,
            north, south, east, west, northeast, northwest, southeast, southwest, mostSignificantBit, difference;
    int slider;

    if (gamestate->whiteToMove) {
        enemyAttacks = (*gamestate->bitboards[6] & ~Board::Files::hFile) >> 7 | (*gamestate->bitboards[6] & ~Board::Files::aFile) >> 9;
        friendlyPieces = gamestate->w_pieces;
        kingPos = gamestate->w_king;
    } else {
        enemyAttacks = (*gamestate->bitboards[0] & ~Board::Files::aFile) << 7 | (*gamestate->bitboards[0] & ~Board::Files::hFile) << 9;
        friendlyPieces = gamestate->b_pieces;
        kingPos = gamestate->b_king;
    }

    allPieces = gamestate->all_pieces ^ kingPos;
    enemyKnights = (*gamestate->bitboards[1] | *gamestate->bitboards[7]) & ~friendlyPieces;
    diagonalSliders = (*gamestate->bitboards[2] | *gamestate->bitboards[4] | *gamestate->bitboards[8] | *gamestate->bitboards[10]) & ~friendlyPieces;
    orthogonalSliders =(*gamestate->bitboards[3] | *gamestate->bitboards[4] | *gamestate->bitboards[9] | *gamestate->bitboards[10]) & ~friendlyPieces;

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
    enemyAttacks |= MovementTables::kingMoves[BitUtils::getLSB(~friendlyPieces & (*gamestate->bitboards[5] | *gamestate->bitboards[11]))];
}

void MoveGenerator::CalculateCheckMask() {
    U64 kingPos;
    kingPos = gamestate->whiteToMove ? gamestate->w_king : gamestate->b_king;
    int kingSquare = BitUtils::getLSB(kingPos);
    king_is_in_double_check = false;

    if (!(enemyAttacks & kingPos)) {
        checkMask = ~0;
        return;
    }

    U64 orthogonalSliders, diagonalSliders, mostSignificantBit, difference, attacks = 0,
            north, south, east, west, northeast, northwest, southeast, southwest;

    if (gamestate->whiteToMove) {
        checkMask = ((gamestate->b_pawn & ~Board::Files::hFile) >> 7 & kingPos) << 7 |
                    ((gamestate->b_pawn & ~Board::Files::aFile) >> 9 & kingPos) << 9;

        checkMask |= MovementTables::knightMoves[kingSquare] & gamestate->b_knight;

        diagonalSliders = gamestate->b_bishop | gamestate->b_queen;
        northwest = gamestate->all_pieces & MovementTables::bishopMoves[kingSquare][0];
        southeast = gamestate->all_pieces & MovementTables::bishopMoves[kingSquare][2];
        northeast = gamestate->all_pieces & MovementTables::bishopMoves[kingSquare][1];
        southwest = gamestate->all_pieces & MovementTables::bishopMoves[kingSquare][3];

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
        orthogonalSliders = gamestate->b_rook | gamestate->b_queen;
        north = gamestate->all_pieces & MovementTables::rookMoves[kingSquare][0];
        south = gamestate->all_pieces & MovementTables::rookMoves[kingSquare][2];
        east = gamestate->all_pieces & MovementTables::rookMoves[kingSquare][1];
        west = gamestate->all_pieces & MovementTables::rookMoves[kingSquare][3];

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
        checkMask = ((gamestate->w_pawn & ~Board::Files::hFile) << 9 & kingPos) >> 9 |
                    ((gamestate->w_pawn & ~Board::Files::aFile) << 7 & kingPos) >> 7;

        checkMask |= MovementTables::knightMoves[kingSquare] & gamestate->w_knight;

        diagonalSliders = gamestate->w_bishop | gamestate->w_queen;
        northwest = gamestate->all_pieces & MovementTables::bishopMoves[kingSquare][0];
        southeast = gamestate->all_pieces & MovementTables::bishopMoves[kingSquare][2];
        northeast = gamestate->all_pieces & MovementTables::bishopMoves[kingSquare][1];
        southwest = gamestate->all_pieces & MovementTables::bishopMoves[kingSquare][3];

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
        orthogonalSliders = gamestate->w_rook | gamestate->w_queen;
        north = gamestate->all_pieces & MovementTables::rookMoves[kingSquare][0];
        south = gamestate->all_pieces & MovementTables::rookMoves[kingSquare][2];
        east = gamestate->all_pieces & MovementTables::rookMoves[kingSquare][1];
        west = gamestate->all_pieces & MovementTables::rookMoves[kingSquare][3];

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
    pinnedPieces = 0;

    U64 kingPos = (gamestate->whiteToMove) ? gamestate->w_king : gamestate->b_king;
    int kingSquare = BitUtils::getLSB(kingPos), potentialPinnedPiece;
    U64 friendlyPieces, enemyDiagonalSliders, enemyOrthogonalSliders, mostSignificantBit, difference, attacks = 0,
            north, south, east, west, northeast, northwest, southeast, southwest, xRayedSquare, enPassantRank,
            potentialPins;

    friendlyPieces = *gamestate->armies[gamestate->whiteToMove];
    enemyDiagonalSliders = (gamestate->w_bishop | gamestate->w_queen | gamestate->b_bishop | gamestate->b_queen) & ~friendlyPieces;
    enemyOrthogonalSliders =(gamestate->w_rook | gamestate->w_queen | gamestate->b_rook | gamestate->b_queen) & ~friendlyPieces;
    enPassantRank = gamestate->whiteToMove ? Board::Ranks::rank_5 : Board::Ranks::rank_4;

    northwest = gamestate->all_pieces & MovementTables::bishopMoves[kingSquare][0];
    southeast = gamestate->all_pieces & MovementTables::bishopMoves[kingSquare][2];
    northeast = gamestate->all_pieces & MovementTables::bishopMoves[kingSquare][1];
    southwest = gamestate->all_pieces & MovementTables::bishopMoves[kingSquare][3];

    mostSignificantBit = BitUtils::getMSB(southeast);
    difference = northwest ^ (northwest - mostSignificantBit);
    attacks |= difference & MovementTables::bishopMoves[kingSquare][4];
    mostSignificantBit = BitUtils::getMSB(southwest);
    difference = northeast ^ (northeast - mostSignificantBit);
    attacks |= difference & MovementTables::bishopMoves[kingSquare][5];

    potentialPins = attacks & friendlyPieces;
    while (potentialPins) {
        potentialPinnedPiece = BitUtils::popLSB(potentialPins);
        xRayedSquare = BitMasks::xRay(kingSquare, potentialPinnedPiece, gamestate->all_pieces);
        if (xRayedSquare & enemyDiagonalSliders) {
            pinnedPieces |= 1ULL << potentialPinnedPiece;
            pinMasks[potentialPinnedPiece] = BitMasks::segmentMask(kingSquare, BitUtils::getLSB(xRayedSquare));
        }
    }

    attacks = 0;
    north = gamestate->all_pieces & MovementTables::rookMoves[kingSquare][0];
    south = gamestate->all_pieces & MovementTables::rookMoves[kingSquare][2];
    east = gamestate->all_pieces & MovementTables::rookMoves[kingSquare][1];
    west = gamestate->all_pieces & MovementTables::rookMoves[kingSquare][3];

    mostSignificantBit = BitUtils::getMSB(south);
    difference = north ^ (north - mostSignificantBit);
    attacks |= difference & MovementTables::rookMoves[kingSquare][4];
    mostSignificantBit = BitUtils::getMSB(west);
    difference = east ^ (east - mostSignificantBit);
    attacks |= difference & MovementTables::rookMoves[kingSquare][5];

    potentialPins = attacks & friendlyPieces;
    while (potentialPins) {
        potentialPinnedPiece = BitUtils::popLSB(potentialPins);
        xRayedSquare = BitMasks::xRay(kingSquare, potentialPinnedPiece, gamestate->all_pieces);
        if (xRayedSquare & enemyOrthogonalSliders) {
            pinnedPieces |= 1ULL << potentialPinnedPiece;
            pinMasks[potentialPinnedPiece] = BitMasks::segmentMask(kingSquare, BitUtils::getLSB(xRayedSquare));
        }
    }

    if (!gamestate->move_log.empty()) {
        if (gamestate->move_log.back().moveFlag & MoveFlags::doublePawnPush &&
            kingPos & enPassantRank &&
            friendlyPieces & (gamestate->w_pawn | gamestate->b_pawn) & enPassantRank &&
            enemyOrthogonalSliders & enPassantRank) {
            for (int potentialPinnedEnPassant : BitUtils::getBits(attacks & (gamestate->w_pawn | gamestate->b_pawn) & (east | west))) {
                xRayedSquare = BitMasks::xRay(kingSquare, potentialPinnedEnPassant, gamestate->all_pieces);
                U64 doubleXRayedSquare = BitMasks::xRay(potentialPinnedEnPassant, BitUtils::getLSB(xRayedSquare), gamestate->all_pieces);
                if ((gamestate->mailbox[BitUtils::getLSB(xRayedSquare)] ^ gamestate->mailbox[potentialPinnedEnPassant]) & 8 &&
                    std::abs(BitUtils::getLSB(xRayedSquare) % 8 - potentialPinnedEnPassant % 8) == 1 &&
                    doubleXRayedSquare & enemyOrthogonalSliders)  {
                    if (gamestate->whiteToMove) {
                        if (gamestate->mailbox[potentialPinnedEnPassant] & 8) {
                            pinnedPieces |= xRayedSquare;
                            pinMasks[BitUtils::getLSB(xRayedSquare)] = ~((1ULL << potentialPinnedEnPassant) << 8);
                        } else {
                            pinnedPieces |= 1ULL << potentialPinnedEnPassant;
                            pinMasks[potentialPinnedEnPassant] = ~(xRayedSquare << 8);
                        }
                    } else {
                        if (gamestate->mailbox[potentialPinnedEnPassant] & 8) {
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
    int king_sq;
    U64 friendly_pieces, to_squares;
    if (gamestate->whiteToMove) {
        king_sq = BitUtils::getLSB(gamestate->w_king);
        friendly_pieces = gamestate->w_pieces;
        if (gamestate->legality & legalityBits::whiteShortCastleMask && gamestate->mailbox[Board::Squares::h1] == 4 && king_sq == Board::Squares::e1 &&
            gamestate->empty_sqs & 1ULL << Board::Squares::f1 && gamestate->empty_sqs & 1ULL << Board::Squares::g1 &&
            !(enemyAttacks & 1ULL << Board::Squares::e1) && !(enemyAttacks & 1ULL << Board::Squares::f1) && !(enemyAttacks & 1ULL << Board::Squares::g1)) {
            legalMoves.emplace_back(king_sq, Board::Squares::g1, MoveFlags::shortCastle);
        }
        if (gamestate->legality & legalityBits::whiteLongCastleMask && gamestate->mailbox[Board::Squares::a1] == 4 && king_sq == Board::Squares::e1 &&
            gamestate->empty_sqs & 1ULL << Board::Squares::d1 && gamestate->empty_sqs & 1ULL << Board::Squares::c1 && gamestate->empty_sqs & 1ULL << Board::Squares::b1 &&
            !(enemyAttacks & 1ULL << Board::Squares::e1) && !(enemyAttacks & 1ULL << Board::Squares::d1) && !(enemyAttacks & 1ULL << Board::Squares::c1)) {
            legalMoves.emplace_back(king_sq, Board::Squares::c1, MoveFlags::longCastle);
        }
    } else {
        king_sq = BitUtils::getLSB(gamestate->b_king);
        friendly_pieces = gamestate->b_pieces;
        if (gamestate->legality & legalityBits::blackShortCastleMask && gamestate->mailbox[Board::Squares::h8] == 12 && king_sq == Board::Squares::e8 &&
            gamestate->empty_sqs & 1ULL << Board::Squares::f8 && gamestate->empty_sqs & 1ULL << Board::Squares::g8 &&
            !(enemyAttacks & 1ULL << Board::Squares::e8) && !(enemyAttacks & 1ULL << Board::Squares::f8) && !(enemyAttacks & 1ULL << Board::Squares::g8)) {
            legalMoves.emplace_back(king_sq, Board::Squares::g8, MoveFlags::shortCastle);
        }
        if (gamestate->legality & legalityBits::blackLongCastleMask && gamestate->mailbox[Board::Squares::a8] == 12 && king_sq == Board::Squares::e8 &&
            gamestate->empty_sqs & 1ULL << Board::Squares::d8 && gamestate->empty_sqs & 1ULL << Board::Squares::c8 && gamestate->empty_sqs & 1ULL << Board::Squares::b8 &&
            !(enemyAttacks & 1ULL << Board::Squares::e8) && !(enemyAttacks & 1ULL << Board::Squares::d8) && !(enemyAttacks & 1ULL << Board::Squares::c8)) {
            legalMoves.emplace_back(king_sq, Board::Squares::c8, MoveFlags::longCastle);
        }
    }

    to_squares = MovementTables::kingMoves[king_sq] & ~friendly_pieces & ~enemyAttacks;
    while (to_squares) {
        int toSquare = BitUtils::popLSB(to_squares);
        legalMoves.emplace_back(king_sq, toSquare, isCapture(*gamestate, toSquare));
    }
}

void MoveGenerator::GeneratePawnMoves() {
    U64 pawnSquares, singleStep, doubleStep, leftCapt, rightCapt, promotion, promotionLeftCapt, promotionRightCapt;
    U64 singleStep_f, doubleStep_f, leftCapt_f, rightCapt_f, promotion_f, promotionLeftCapt_f, promotionRightCapt_f;
    U64 promotionRank, enPassantRank, oneStepRank;
    U64 enemyPieces, enPassant_f = 0ULL, enPassantSquare = 0ULL, enPassantCheckMask = 0ULL;
    int fromSq, toSq;

    if (gamestate->whiteToMove) {
        pawnSquares = gamestate->w_pawn;
        promotionRank = Board::Ranks::rank_7;
        enPassantRank = Board::Ranks::rank_6;
        oneStepRank = Board::Ranks::rank_3;
        enemyPieces = gamestate->b_pieces;
    } else /* black to move */ {
        pawnSquares = gamestate->b_pawn;
        promotionRank = Board::Ranks::rank_2;
        enPassantRank = Board::Ranks::rank_3;
        oneStepRank = Board::Ranks::rank_6;
        enemyPieces = gamestate->w_pieces;
    }

    singleStep = PawnMoves::oneStep(gamestate->whiteToMove, pawnSquares & ~promotionRank) & gamestate->empty_sqs & checkMask;
    doubleStep = PawnMoves::twoStep(gamestate->whiteToMove, pawnSquares & (PawnMoves::oneStep(!gamestate->whiteToMove, oneStepRank & gamestate->empty_sqs))) & gamestate->empty_sqs & checkMask;
    leftCapt = PawnMoves::leftwardCapt(gamestate->whiteToMove, pawnSquares & ~promotionRank) & enemyPieces & checkMask;
    rightCapt = PawnMoves::rightwardCapt(gamestate->whiteToMove, pawnSquares & ~promotionRank) & enemyPieces & checkMask;
    promotion = PawnMoves::oneStep(gamestate->whiteToMove, pawnSquares & promotionRank) & gamestate->empty_sqs & checkMask;
    promotionLeftCapt = PawnMoves::leftwardCapt(gamestate->whiteToMove, pawnSquares & promotionRank) & enemyPieces & checkMask;
    promotionRightCapt = PawnMoves::rightwardCapt(gamestate->whiteToMove, pawnSquares & promotionRank) & enemyPieces & checkMask;

    singleStep_f = PawnMoves::oneStep(!gamestate->whiteToMove, singleStep);
    doubleStep_f = PawnMoves::twoStep(!gamestate->whiteToMove, doubleStep);
    leftCapt_f = PawnMoves::leftwardCapt(!gamestate->whiteToMove, leftCapt);
    rightCapt_f = PawnMoves::rightwardCapt(!gamestate->whiteToMove, rightCapt);
    promotion_f = PawnMoves::oneStep(!gamestate->whiteToMove, promotion);
    promotionLeftCapt_f = PawnMoves::leftwardCapt(!gamestate->whiteToMove, promotionLeftCapt);
    promotionRightCapt_f = PawnMoves::rightwardCapt(!gamestate->whiteToMove, promotionRightCapt);

    if (gamestate->legality & legalityBits::enPassantLegalMask) {
        enPassantSquare = Board::Files::aFile << ((gamestate->legality & legalityBits::enPassantFileMask) >> legalityBits::enPassantFileShift) & enPassantRank;
        enPassantCheckMask = PawnMoves::oneStep(gamestate->whiteToMove, (checkMask & PawnMoves::oneStep(!gamestate->whiteToMove, enPassantSquare)));
        enPassant_f = PawnMoves::allCaptures(!gamestate->whiteToMove, enPassantSquare & (checkMask | enPassantCheckMask)) & pawnSquares;
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
    U64 friendlyPieces, toSquares, knightSquares;
    int knightSq, toSquare;
    if (gamestate->whiteToMove) {
        knightSquares = gamestate->w_knight;
        friendlyPieces = gamestate->w_pieces;
    } else {
        knightSquares = gamestate->b_knight;
        friendlyPieces = gamestate->b_pieces;
    }

    while (knightSquares) {
        knightSq = BitUtils::popLSB(knightSquares);
        toSquares = MovementTables::knightMoves[knightSq] & ~friendlyPieces & checkMask;
        while (toSquares) {
            toSquare = BitUtils::popLSB(toSquares);
            legalMoves.emplace_back(knightSq, toSquare, isCapture(*gamestate, toSquare));
        }
    }
}

void MoveGenerator::GenerateBishopMoves() {
    U64 friendlyPieces, sliders;
    U64 northwest, southwest, northeast, southeast;
    U64 most_significant_bit, difference, target_squares = 0;
    int slider;

    if (gamestate->whiteToMove) {
        friendlyPieces = gamestate->w_pieces;
        sliders = gamestate->w_bishop | gamestate->w_queen;
    } else /* black to move */ {
        friendlyPieces = gamestate->b_pieces;
        sliders = gamestate->b_bishop | gamestate->b_queen;
    }

    while (sliders) {
        slider = BitUtils::popLSB(sliders);

        northwest = gamestate->all_pieces & MovementTables::bishopMoves[slider][0];
        southeast = gamestate->all_pieces & MovementTables::bishopMoves[slider][2];
        northeast = gamestate->all_pieces & MovementTables::bishopMoves[slider][1];
        southwest = gamestate->all_pieces & MovementTables::bishopMoves[slider][3];

        most_significant_bit = BitUtils::getMSB(southeast);
        difference = northwest ^ (northwest - most_significant_bit);
        target_squares |= difference & MovementTables::bishopMoves[slider][4] & ~friendlyPieces & checkMask;

        most_significant_bit = BitUtils::getMSB(southwest);
        difference = northeast ^ (northeast - most_significant_bit);
        target_squares |= difference & MovementTables::bishopMoves[slider][5] & ~friendlyPieces & checkMask;

        while (target_squares) {
            int toSquare = BitUtils::popLSB(target_squares);
            legalMoves.emplace_back(slider, toSquare, isCapture(*gamestate, toSquare));
        }
    }
}

void MoveGenerator::GenerateRookMoves() {
    U64 friendlyPieces, sliders;
    U64 north, south, east, west;
    U64 most_significant_bit, difference, target_squares = 0;
    int slider;

    if (gamestate->whiteToMove) {
        friendlyPieces = gamestate->w_pieces;
        sliders = gamestate->w_rook | gamestate->w_queen;
    } else {
        friendlyPieces = gamestate->b_pieces;
        sliders = gamestate->b_rook | gamestate->b_queen;
    }

    while(sliders) {
        slider = BitUtils::popLSB(sliders);

        north = gamestate->all_pieces & MovementTables::rookMoves[slider][0];
        south = gamestate->all_pieces & MovementTables::rookMoves[slider][2];
        east = gamestate->all_pieces & MovementTables::rookMoves[slider][1];
        west = gamestate->all_pieces & MovementTables::rookMoves[slider][3];

        most_significant_bit = BitUtils::getMSB(south);
        difference = north ^ (north - most_significant_bit);
        target_squares |= difference & MovementTables::rookMoves[slider][4] & ~friendlyPieces & checkMask;

        most_significant_bit = BitUtils::getMSB(west);
        difference = east ^ (east - most_significant_bit);
        target_squares |= difference & MovementTables::rookMoves[slider][5] & ~friendlyPieces & checkMask;

        while (target_squares) {
            int toSquare = BitUtils::popLSB(target_squares);
            legalMoves.emplace_back(slider, toSquare, isCapture(*gamestate, toSquare));
        }
    }
}

int MoveGenerator::PerftTree(int depthPly) {
    if (depthPly == 1) {
        GenerateLegalMoves();
        return static_cast<int>(legalMoves.size());
    }

    GenerateLegalMoves();
    int nodesFound = 0;
    std::vector<Move> currentLegalMoves = legalMoves;
    for (auto move : currentLegalMoves) {
        gamestate->makeMove(move);
        nodesFound += PerftTree(depthPly - 1);
        gamestate->undoMove();
    }

    return nodesFound;
}

void MoveGenerator::PerftTest() {
    GAMESTATE position1 = GAMESTATE("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    GAMESTATE position2 = GAMESTATE("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");
    GAMESTATE position3 = GAMESTATE("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");
    GAMESTATE position4 = GAMESTATE("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1");
    GAMESTATE position5 = GAMESTATE("rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8");
    GAMESTATE position6 = GAMESTATE("r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10");
    float totalTime = 0, averageNPS;

    Seed(position1);
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

    Seed(position2);
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

    Seed(position3);
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

    Seed(position4);
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

    Seed(position5);
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

    Seed(position6);
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
//*/