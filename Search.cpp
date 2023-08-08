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

int MovePicker::MiniMaxSearch(int depth, int alpha, int beta) {
    if (depth == 0) {
        return Evaluator::Get().StaticEvaluation();
    }

    MoveGenerator::Get().GenerateLegalMoves();
    std::vector<Move> legalMoves = MoveGenerator::Get().legalMoves;
    if (Gamestate::Get().whiteToMove) {
        int maxEval = constantEvals::negativeInfinity;
        for (Move move: legalMoves) {
            Gamestate::Get().MakeMove(move);
            int eval = MiniMaxSearch(depth - 1, alpha, beta);
            Gamestate::Get().UndoMove();
            if (depth == 4 && eval > maxEval) bestMove = move;
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha) break;
        }
        return maxEval;
    } else /* black to move */ {
        int minEval = constantEvals::positiveInfinity;
        for (Move move: legalMoves) {
            Gamestate::Get().MakeMove(move);
            int eval = MiniMaxSearch(depth - 1, alpha, beta);
            Gamestate::Get().UndoMove();
            if (depth == 4 && eval < minEval) bestMove = move;
            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);
            if (beta <= alpha) break;

        }
        return minEval;
    }
}
