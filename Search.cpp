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
    auto start = std::chrono::steady_clock::now();
    for (; searchDepth <= maxDepth; searchDepth += 2) {
        bestEvalThisIteration = Gamestate::Get().whiteToMove ? -Infinity : Infinity;
        NegaMaxSearch(searchDepth, 0, -Infinity, Infinity);
        bestMove = bestMoveThisIteration;
        bestEval = bestEvalThisIteration;

        if (isMateEval(bestEval)) {
            break;
        }

        auto elapsed = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed - start).count() > 10) {
            break;
        }
    }
    std::cout << searchDepth << std::endl;
}

int MovePicker::NegaMaxSearch(int depth, int depthFromRoot, int alpha, int beta) {
    if (depthFromRoot > 0) {
        alpha = std::max(alpha, -Infinity + depthFromRoot);
        beta = std::min(beta, Infinity - depthFromRoot);
    }

    int transpositionEval = TranspositionTable::Get().Lookup(depth, depthFromRoot, alpha, beta);
    if (transpositionEval != LookUpFailed) {
        if (depthFromRoot == 0) {
            bestEvalThisIteration = transpositionEval;
            bestMoveThisIteration = TranspositionTable::Get().positions.at(Gamestate::Get().zobristKey).bestMove;
        }
        return transpositionEval;
    }

    if (depth == 0) {
        return QuiessenceSearch(alpha, beta);
    }

    Gamestate &gamestate = Gamestate::Get();

    std::vector<Move> legalMoves = MoveGenerator::Get().GenerateLegalMoves();
    MoveOrderer::Get().OrderMoves(&legalMoves);

    if (MoveGenerator::Get().king_is_in_check) {
        depth += 1;
    }

    if (legalMoves.empty()) {
        if (MoveGenerator::Get().king_is_in_check) {
            return -(Infinity - depthFromRoot);
        }
        return 0;
    }

    Move currentBestMove = legalMoves[0];
    EvaluationType currentType = BestCase;

    for (Move move: legalMoves) {

        gamestate.MakeMove(move);
        int eval = -NegaMaxSearch(depth - 1, depthFromRoot + 1, -beta, -alpha);
        gamestate.UndoMove();

        if (eval >= beta) {
            TranspositionTable::Get().StorePosition(depth, depthFromRoot, beta, WorstCase, move);
            return beta;
        }

        if (eval > alpha) {
            currentBestMove = move;
            currentType = Exact;
            alpha = eval;

            if (depthFromRoot == 0) {
                bestEvalThisIteration = eval;
                bestMoveThisIteration = move;
            }
        }
        if (beta <= alpha) break;
    }

    TranspositionTable::Get().StorePosition(depth, depthFromRoot, alpha, currentType, currentBestMove);
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
