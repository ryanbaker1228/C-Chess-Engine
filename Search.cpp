//
// Created by Ryan Baker on 7/29/23.
//

#include "Search.h"
#include "evaluation.h"

MovePicker::MovePicker() {

}

void MovePicker::InitSearch() {
    bestEval = Gamestate::Get().whiteToMove ? constantEvals::blackWin : constantEvals::whiteWin;
    MoveGenerator& generator = MoveGenerator::Get();
    int eval;

    generator.GenerateLegalMoves();
    for (Move move : generator.legalMoves) {
        Gamestate::Get().MakeMove(move);
        eval = Evaluator::Get().StaticEvaluation();
        if (eval < bestEval) {
            bestMove = move;
            bestEval = eval;
        }
        Gamestate::Get().UndoMove();
    }
}

int MovePicker::MiniMaxSearch(int depth, int depthFromRoot, int alpha, int beta) {
    if (depth == 0) {
        return Evaluator::Get().StaticEvaluation();
    }
    Gamestate& gamestate = Gamestate::Get();

    MoveOrderer::Get().OrderMoves();
    std::vector<Move> orderedMoves = MoveGenerator::Get().legalMoves;

    if (orderedMoves.empty()) {
        if (MoveGenerator::Get().king_is_in_check) {
            return (Gamestate::Get().whiteToMove ? constantEvals::negativeInfinity : constantEvals::positiveInfinity);
        }
        return constantEvals::draw;
    }

    if (gamestate.whiteToMove) {
        int maxEval = constantEvals::negativeInfinity;
        for (Move move: orderedMoves) {
            gamestate.MakeMove(move);
            int eval = MiniMaxSearch(depth - 1, depthFromRoot + 1, alpha, beta);
            gamestate.UndoMove();
            if (depthFromRoot == 0 && eval > maxEval) {
                bestMove = move;
                bestEval = eval;
            }
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha) break;
        }
        return maxEval;
    } else /* black to move */ {
        int minEval = constantEvals::positiveInfinity;
        for (Move move: orderedMoves) {
            gamestate.MakeMove(move);
            int eval = MiniMaxSearch(depth - 1, depthFromRoot + 1, alpha, beta);
            gamestate.UndoMove();
            if (depthFromRoot == 0 && eval < minEval) {
                bestMove = move;
                bestEval = eval;
            }
            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);
            if (beta <= alpha) break;

        }
        return minEval;
    }
}

void MovePicker::IterativeDeepeningSearch() {
    for (int depth = 0; depth < maxDepth; ++depth) {
        MiniMaxSearch(depth, 0, constantEvals::negativeInfinity, constantEvals::positiveInfinity);
    }
}

MoveOrderer::MoveOrderer() {

}

void MoveOrderer::OrderMoves() {
    MoveGenerator::Get().GenerateLegalMoves();
    //ComputeGuardHeuristic();

    std::sort(MoveGenerator::Get().legalMoves.begin(), MoveGenerator::Get().legalMoves.end(), std::greater<>());

}

int MoveOrderer::Promise(Move move) {

    int promise = 0;
    int movingPiece = Gamestate::Get().mailbox[move.startSquare];
    int capturedPiece = Gamestate::Get().mailbox[move.endSquare];

    if (move.moveFlag & MoveFlags::capture) {
        promise += EvaluatePiece(capturedPiece & 0b0111);
    }

    if (move.moveFlag & MoveFlags::promotion) {
        promise += EvaluatePiece(PromotingPiece.at(move.moveFlag));
    }

    if ((1ULL << move.endSquare) & PawnMoves::allCaptures(!Gamestate::Get().whiteToMove, EnemyPawns())) {
        promise -= EvaluatePiece(movingPiece & 0b0111);
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
