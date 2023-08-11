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

void MovePicker::IterativeDeepeningSearch() {
    auto start = std::chrono::steady_clock::now();
    int depth = 1;
    for (; depth < maxDepth; ++depth) {
        NegaMaxSearch(depth, 0, constantEvals::negativeInfinity, constantEvals::positiveInfinity);
        auto elapsed = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::microseconds>(elapsed - start).count() >= 100000) {
            break;
        }
    }
    std::cout << depth << std::endl;
}
/*
int MovePicker::MiniMaxSearch(int depth, int depthFromRoot, int alpha, int beta) {
    if (depth == 0) {
        return Evaluator::Get().StaticEvaluation();
    }

    Gamestate &gamestate = Gamestate::Get();

    MoveOrderer::Get().OrderMoves(depthFromRoot);
    std::vector<Move> orderedMoves = MoveGenerator::Get().legalMoves;

    if (orderedMoves.empty()) {
        if (MoveGenerator::Get().king_is_in_check) {
            return (Gamestate::Get().whiteToMove ? constantEvals::negativeInfinity : constantEvals::positiveInfinity);
        }
        return constantEvals::draw;
    }

    if (gamestate.whiteToMove) {
        for (Move move : orderedMoves) {
            gamestate.MakeMove(move);
            int eval = MiniMaxSearch(depth - 1, depthFromRoot + 1, alpha, beta);
            gamestate.UndoMove();

            if (eval >= beta) {
                return beta;
            }

            if (eval > alpha) {
                alpha = eval;

                if (depthFromRoot == 0) {
                    bestEval = eval;
                    bestMove = move;
                }
            }
            if (beta <= alpha) break;
        }
        return alpha;

    } else  black to move  {
        for (Move move : orderedMoves) {
            gamestate.MakeMove(move);
            int eval = MiniMaxSearch(depth - 1, depthFromRoot + 1, alpha, beta);
            gamestate.UndoMove();

            if (eval <= alpha) {
                return alpha;
            }

            if (eval < beta) {
                beta = eval;

                if (depthFromRoot == 0) {
                    bestEval = eval;
                    bestMove = move;
                }
            }
            if (beta <= alpha) break;
        }
        return beta;
    }
}
*/
int MovePicker::NegaMaxSearch(int depth, int depthFromRoot, int alpha, int beta) {
    if (depth == 0) {
        return Evaluator::Get().StaticEvaluation();
    }

    if (depthFromRoot > 0) {
        alpha = std::max(alpha, -99999 + depthFromRoot);
        beta = std::min(beta, 99999 - depthFromRoot);
    }
    Gamestate &gamestate = Gamestate::Get();

    MoveOrderer::Get().OrderMoves(depthFromRoot);
    std::vector<Move> orderedMoves = MoveGenerator::Get().legalMoves;

    if (orderedMoves.empty()) {
        if (MoveGenerator::Get().king_is_in_check) {
            return -(99999 - depthFromRoot);
        }
        return 0;
    }

    for (Move move : orderedMoves) {
        gamestate.MakeMove(move);
        int eval = -NegaMaxSearch(depth - 1, depthFromRoot + 1, -beta, -alpha);
        gamestate.UndoMove();

        if (eval >= beta) {
            return beta;
        }

        if (eval > alpha) {
            alpha = eval;

            if (depthFromRoot == 0) {
                bestEval = eval;
                bestMove = move;
            }
        }

        if (beta <= alpha) break;
    }
    return alpha;
}

MoveOrderer::MoveOrderer() {

}

void MoveOrderer::OrderMoves(int depth) {
    MoveGenerator::Get().GenerateLegalMoves();

    currentDepth = depth;
    enemyPawnAttacks = PawnMoves::allCaptures(!Gamestate::Get().whiteToMove, EnemyPawns());
    //ComputeGuardHeuristic();
    ++c;
    std::sort(MoveGenerator::Get().legalMoves.begin(), MoveGenerator::Get().legalMoves.end(), std::greater<>());
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
