//
// Created by Ryan Baker on 7/29/23.
//

#include "Search.h"
#include "evaluation.h"
#include <thread>

MovePicker::MovePicker() {

}

void MovePicker::InitSearch() {
    int searchDepth = 2;
    start = std::chrono::steady_clock::now();
    abortSearch = false;
    while (true) {
        bestEvalThisIteration = Gamestate::Get().whiteToMove ? -Infinity : Infinity;
        int curr_eval = NegaMaxSearch(searchDepth, 0, -Infinity, Infinity);

        if (abortSearch) {
            if (curr_eval > bestEval) {
                bestEval = bestEvalThisIteration;
                bestMove = bestMoveThisIteration;
            }
            break;
        }

        bestMove = bestMoveThisIteration;
        bestEval = bestEvalThisIteration;

        if (isMateEval(bestEval)) {
            break;
        }

        searchDepth += 2;
    }
}

int MovePicker::NegaMaxSearch(int depth_to_search, int depth_from_root, int alpha, int beta) {
    abortSearch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start).count() > search_time;
    if (abortSearch) {
        return alpha;
    }

    if (depth_from_root > 0) {
        alpha = std::max(alpha, -Infinity + depth_from_root);
        beta = std::min(beta, Infinity - depth_from_root);
    }

    int transposition_eval = TranspositionTable::Get().Lookup(depth_to_search, depth_from_root, alpha, beta);
    if (transposition_eval != LookUpFailed) {
        if (depth_from_root == 0) {
            bestEvalThisIteration = transposition_eval;
            bestMoveThisIteration = TranspositionTable::Get().positions.at(Gamestate::Get().zobristKey).bestMove;
        }
        return transposition_eval;
    }

    if (depth_to_search == 0) {
        int eval = QuiessenceSearch(alpha, beta);
        TranspositionTable::Get().StorePosition(depth_to_search, depth_from_root, eval, Exact, {0, 0, 0});
        return eval;
    }

    Gamestate& gamestate = Gamestate::Get();

    std::vector<Move> legal_moves = MoveGenerator::Get().GenerateLegalMoves();
    MoveOrderer::Get().OrderMoves(&legal_moves);

    if (MoveGenerator::Get().king_is_in_check) {
        ++depth_to_search;
    }

    if (legal_moves.empty()) {
        if (MoveGenerator::Get().king_is_in_check) {
            return -Infinity + depth_from_root;
        }
        return 0;
    }

    Move current_best_move = legal_moves[0];
    EvaluationType type = BestCase;

    for (Move move : legal_moves) {
        gamestate.MakeMove(move);
        int eval = -NegaMaxSearch(depth_to_search - 1, depth_from_root + 1, -beta, -alpha);
        gamestate.UndoMove();

        if (eval >= beta) {
            TranspositionTable::Get().StorePosition(depth_to_search, depth_from_root, beta, WorstCase, move);
            return beta;
        }

        if (eval > alpha) {
            current_best_move = move;
            type = Exact;
            alpha = eval;

            if (depth_from_root == 0) {
                bestEvalThisIteration = eval;
                bestMoveThisIteration = move;
            }
        }

        if (beta <= alpha) break;
    }

    TranspositionTable::Get().StorePosition(depth_to_search, depth_from_root, alpha, type, current_best_move);
    return alpha;
}

int MovePicker::QuiessenceSearch(int alpha, int beta) {
    int current_eval = Evaluator::Get().StaticEvaluation();

    if (current_eval >= beta) return beta;
    if (current_eval > alpha) alpha = current_eval;

    Gamestate &gamestate = Gamestate::Get();
    std::vector<Move> legalMoves = MoveGenerator::Get().GenerateLegalMoves(true);
    MoveOrderer::Get().OrderMoves(&legalMoves);

    for (Move move : legalMoves) {
        gamestate.MakeMove(move);
        int eval = -QuiessenceSearch(-beta, -alpha);
        gamestate.UndoMove();

        if (eval >= beta) {
            return beta;
        }

        if (eval > alpha) {
            alpha = eval;
        }

        if (beta <= alpha) break;
    }

    return alpha;
}

MoveOrderer::MoveOrderer() {

}

void MoveOrderer::OrderMoves(std::vector<Move>* legalMoves) {
    if (!legalMoves->size()) {
        return;
    }

    enemyPawnAttacks = PawnMoves::allCaptures(!Gamestate::Get().whiteToMove, EnemyPawns());
    //ComputeGuardHeuristic();

    std::sort(legalMoves->begin(), legalMoves->end(), std::greater<>());
}

int MoveOrderer::Promise(Move move) {
    int promise = 0;
    int movingPiece = Gamestate::Get().mailbox[move.startSquare];
    int capturedPiece = Gamestate::Get().mailbox[move.endSquare];

    if (move.flag == MoveFlags::enPassant) {
        capturedPiece = Gamestate::Get().whiteToMove ? 9 : 1;
    }

    if (move.flag & MoveFlags::capture) {
        promise += EvaluatePiece(capturedPiece) + GuardScores.at(movingPiece);
    }

    if (move.flag & MoveFlags::promotion) {
        promise += EvaluatePiece(PromotingPiece.at(move.flag));
    }

    if ((1ULL << move.endSquare) & enemyPawnAttacks) {
        promise -= EvaluatePiece(movingPiece);
    }

    if (Gamestate::Get().legality & legalityBits::capturedPieceMask &&
        Gamestate::Get().moveLog.top().endSquare == move.endSquare) {
        promise += EvaluatePiece(capturedPiece);
    }

    if (move.startSquare == MovePicker::Get().bestMoveThisIteration.startSquare &&
        move.endSquare == MovePicker::Get().bestMoveThisIteration.endSquare) {
        promise += pow(10, 6);
    }

    if (!(movingPiece & 0b1000)) {
        promise += PcSqTables::midGameTables[PieceNum2BitboardIndex.at(movingPiece)][move.endSquare] -
                   PcSqTables::midGameTables[PieceNum2BitboardIndex.at(movingPiece)][move.startSquare];
    } else {
        promise -= PcSqTables::midGameTables[PieceNum2BitboardIndex.at(movingPiece)][move.endSquare] -
                   PcSqTables::midGameTables[PieceNum2BitboardIndex.at(movingPiece)][move.startSquare];
    }

    return promise;
}

void MoveOrderer::ComputeGuardHeuristic() {
    GuardValues = {0};
    const Gamestate &gamestate = Gamestate::Get();
    U64 enemyLeftPawnAttacks, enemyRightPawnAttacks, leftPawnDefense, rightPawnDefense,
        enemyKnights, friendlyKnights,
        enemyBishops, friendlyBishops,
        enemyRooks, friendlyRooks,
        enemyKing, enemyKingAttacks, friendlyKing, kingDefense,
        attackedSqs, most_significant_bit, difference,
        northwest, northeast, southwest, southeast,
        north, west, south, east;
    int slider;

    enemyLeftPawnAttacks = PawnMoves::leftwardCapt(!gamestate.whiteToMove, EnemyPawns());
    enemyRightPawnAttacks = PawnMoves::rightwardCapt(!gamestate.whiteToMove, EnemyPawns());
    leftPawnDefense = PawnMoves::leftwardCapt(gamestate.whiteToMove, FriendlyPawns());
    rightPawnDefense = PawnMoves::rightwardCapt(gamestate.whiteToMove, FriendlyPawns());
    enemyKnights = EnemyKnights();
    friendlyKnights = FriendlyKnights();
    enemyBishops = EnemyBishops() | EnemyQueen();
    friendlyBishops = FriendlyBishops() | FriendlyQueen();
    enemyRooks = EnemyRooks() | EnemyQueen();
    friendlyRooks = FriendlyRooks() | FriendlyQueen();
    enemyKing = EnemyKing();
    friendlyKing = FriendlyKing();
    enemyKingAttacks = MovementTables::kingMoves[getLSB(enemyKing)];
    kingDefense = MovementTables::kingMoves[getLSB(friendlyKing)];

    while (enemyLeftPawnAttacks) GuardValues[popLSB(enemyLeftPawnAttacks)] -= GuardScores.at(pawn);
    while (enemyRightPawnAttacks) GuardValues[popLSB(enemyRightPawnAttacks)] -= GuardScores.at(pawn);
    while (leftPawnDefense) GuardValues[popLSB(leftPawnDefense)] += GuardScores.at(pawn);
    while (rightPawnDefense) GuardValues[popLSB(rightPawnDefense)] += GuardScores.at(pawn);

    while (enemyKnights) {
        attackedSqs = MovementTables::knightMoves[popLSB(enemyKnights)];
        while (attackedSqs) GuardValues[popLSB(attackedSqs)] -= GuardScores.at(knight);
    }
    while (friendlyKnights) {
        attackedSqs = MovementTables::knightMoves[popLSB(friendlyKnights)];
        while (attackedSqs) GuardValues[popLSB(attackedSqs)] += GuardScores.at(knight);
    }

    while (enemyBishops) {
        slider = popLSB(enemyBishops);

        northwest = gamestate.all_pieces & MovementTables::bishopMoves[slider][0];
        southeast = gamestate.all_pieces & MovementTables::bishopMoves[slider][2];
        northeast = gamestate.all_pieces & MovementTables::bishopMoves[slider][1];
        southwest = gamestate.all_pieces & MovementTables::bishopMoves[slider][3];

        most_significant_bit = getMSB(southeast);
        difference = northwest ^ (northwest - most_significant_bit);
        attackedSqs |= difference & MovementTables::bishopMoves[slider][4] & ~EnemyPieces();

        most_significant_bit = getMSB(southwest);
        difference = northeast ^ (northeast - most_significant_bit);
        attackedSqs |= difference & MovementTables::bishopMoves[slider][5] & ~EnemyPieces();

        while (attackedSqs) {
            int toSquare = popLSB(attackedSqs);
            GuardValues[toSquare] -= isQueen(toSquare) ? GuardScores.at(queen) : GuardScores.at(bishop);
        }
    }
    while (friendlyBishops) {
        slider = popLSB(friendlyBishops);

        northwest = gamestate.all_pieces & MovementTables::bishopMoves[slider][0];
        southeast = gamestate.all_pieces & MovementTables::bishopMoves[slider][2];
        northeast = gamestate.all_pieces & MovementTables::bishopMoves[slider][1];
        southwest = gamestate.all_pieces & MovementTables::bishopMoves[slider][3];

        most_significant_bit = getMSB(southeast);
        difference = northwest ^ (northwest - most_significant_bit);
        attackedSqs |= difference & MovementTables::bishopMoves[slider][4] & ~FriendlyPieces();

        most_significant_bit = getMSB(southwest);
        difference = northeast ^ (northeast - most_significant_bit);
        attackedSqs |= difference & MovementTables::bishopMoves[slider][5] & ~FriendlyPieces();

        while (attackedSqs) {
            int toSquare = popLSB(attackedSqs);
            GuardValues[toSquare] += isQueen(slider) ? GuardScores.at(queen) : GuardScores.at(bishop);
        }
    }

    while (enemyRooks) {
        slider = popLSB(enemyRooks);

        north = gamestate.all_pieces & MovementTables::rookMoves[slider][0];
        south = gamestate.all_pieces & MovementTables::rookMoves[slider][2];
        east = gamestate.all_pieces & MovementTables::rookMoves[slider][1];
        west = gamestate.all_pieces & MovementTables::rookMoves[slider][3];

        most_significant_bit = getMSB(south);
        difference = north ^ (north - most_significant_bit);
        attackedSqs |= difference & MovementTables::rookMoves[slider][4] & ~EnemyPieces();

        most_significant_bit = getMSB(west);
        difference = east ^ (east - most_significant_bit);
        attackedSqs |= difference & MovementTables::rookMoves[slider][5] & ~EnemyPieces();

        while (attackedSqs) {
            int toSquare = popLSB(attackedSqs);
            GuardValues[toSquare] -= isQueen(slider) ? GuardScores.at(queen) : GuardScores.at(rook);
        }
    }
    while (friendlyRooks) {
        slider = popLSB(friendlyRooks);

        north = gamestate.all_pieces & MovementTables::rookMoves[slider][0];
        south = gamestate.all_pieces & MovementTables::rookMoves[slider][2];
        east = gamestate.all_pieces & MovementTables::rookMoves[slider][1];
        west = gamestate.all_pieces & MovementTables::rookMoves[slider][3];

        most_significant_bit = getMSB(south);
        difference = north ^ (north - most_significant_bit);
        attackedSqs |= difference & MovementTables::rookMoves[slider][4] & ~FriendlyPieces();

        most_significant_bit = getMSB(west);
        difference = east ^ (east - most_significant_bit);
        attackedSqs |= difference & MovementTables::rookMoves[slider][5] & ~FriendlyPieces();

        while (attackedSqs) {
            int toSquare = popLSB(attackedSqs);
            GuardValues[toSquare] += isQueen(slider) ? GuardScores.at(queen) : GuardScores.at(rook);
        }
    }

    while (enemyKingAttacks) GuardValues[popLSB(enemyKingAttacks)] -= GuardScores.at(king);
    while (kingDefense) GuardValues[popLSB(kingDefense)] += GuardScores.at(king);
}
