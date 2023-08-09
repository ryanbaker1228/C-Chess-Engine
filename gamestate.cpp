//
// Created by Ryan Baker on 6/13/23.
//
#include "gamestate.h"
#include "move.h"
#include "movegen.h"
#include "bitUtils.h"

Gamestate::Gamestate() {
    Seed();
}

void Gamestate::Seed(const std::string& position) {
    InitFENString(position);
    InitBitboards();
    while(!moveLog.empty()) moveLog.pop();
    while(!legalityHistory.empty()) legalityHistory.pop();
}

void Gamestate::InitFENString(const std::string &position) {
    enum FENStringFields {
        PiecePlacement,
        PlayerToMove,
        CastlingRights,
        EnPassantPossible,
        HalfMoveClock,
        FullMoveCount,
    };
    int currentField = PiecePlacement;
    int row = 7;
    int col = 0;

    for (char c : position) {
        switch (currentField) {
            case PiecePlacement:
                if (c == ' ') {
                    ++currentField;
                    break;
                }
                if (c == '/') {
                    --row;
                    col = 0;
                    break;
                }
                if (isnumber(c)) {
                    int endCol = col + c - '0';
                    for (; col < endCol; ++col) {
                        mailbox[8 * row + col] = 0;
                    }
                    break;
                }
                mailbox[8 * row + col] = PieceChar2Number.at(c);
                ++col;
                break;

            case PlayerToMove:
                if (c == ' ') {
                    ++currentField;
                    break;
                }
                whiteToMove = c == 'w';
                break;

            case CastlingRights:
                if (c == ' ') {
                    ++currentField;
                    break;
                }
                legality |= CastlingChar2LegalityMask.at(c);
                break;

            case EnPassantPossible:
                if (c == ' ') {
                    ++currentField;
                    break;
                }
                if (c == '-') {
                    break;
                }
                if (isnumber(c)) {
                    legality |= legalityBits::enPassantLegalMask;
                    legality |= col << legalityBits::enPassantFileShift;
                    break;
                }
                col = c - 'a';
                break;

            case HalfMoveClock:
            case FullMoveCount:
            default:
                break;
        }
    }
}

void Gamestate::InitBitboards() {
    w_pawn = w_knight = w_bishop = w_rook = w_queen = w_king = 0;
    b_pawn = b_knight = b_bishop = b_rook = b_queen = b_king = 0;

    for (int square = 0; square < 64; ++square) {
        if (!mailbox[square]) continue;
        *bitboards[PieceNum2BitboardIndex.at(mailbox[square])] |= 1ULL << square;
    }
    w_pieces = w_pawn | w_knight | w_bishop | w_rook | w_queen | w_king;
    b_pieces = b_pawn | b_knight | b_bishop | b_rook | b_queen | b_king;
    all_pieces = w_pieces | b_pieces;
    empty_sqs = ~all_pieces;
}

void Gamestate::MakeMove(Move move) {
    legalityHistory.push(legality);
    moveLog.push(move);

    int movingPiece = mailbox[move.startSquare];
    int capturedPiece = mailbox[move.endSquare];
    U64 moveSquares = (1ULL << move.startSquare | 1ULL << move.endSquare);

    legality = legalityHistory.top() & legalityBits::castleMask;
    legality |= capturedPiece << legalityBits::capturedPieceShift;

    switch (move.moveFlag) {
        case MoveFlags::quietMove:
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= moveSquares;
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece;
            break;
        case MoveFlags::doublePawnPush:
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= moveSquares;
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece;
            legality |= legalityBits::enPassantLegalMask;
            legality |= (move.endSquare % 8) << legalityBits::enPassantFileShift;
            break;
        case MoveFlags::shortCastle:
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= moveSquares;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece) - 2] ^= (1ULL << (move.startSquare + 1) | 1ULL << (move.endSquare + 1));
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece;
            mailbox[move.startSquare + 1] = movingPiece - 2;
            mailbox[move.endSquare + 1] = 0;
            break;
        case MoveFlags::longCastle:
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= moveSquares;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece) - 2] ^= (1ULL << (move.startSquare - 1) | 1ULL << (move.endSquare - 2));
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece;
            mailbox[move.startSquare - 1] = movingPiece - 2;
            mailbox[move.endSquare - 2] = 0;
            break;
        case MoveFlags::capture:
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= moveSquares;
            *bitboards[PieceNum2BitboardIndex.at(capturedPiece)] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece;
            break;
        case MoveFlags::enPassant:
            int startRow, endCol, capturedSquare;
            startRow = move.startSquare / 8;
            endCol = move.endSquare % 8;
            capturedSquare = 8 * startRow + endCol;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= moveSquares;
            *bitboards[PieceNum2BitboardIndex.at(mailbox[capturedSquare])] ^= 1ULL << capturedSquare;
            mailbox[move.startSquare] = mailbox[capturedSquare] = 0;
            mailbox[move.endSquare] = movingPiece;
            break;
        case MoveFlags::knightPromotion:
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece) + 1] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece + 1;
            break;
        case MoveFlags::bishopPromotion:
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece) + 2] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece + 2;
            break;
        case MoveFlags::rookPromotion:
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece) + 3] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece + 3;
            break;
        case MoveFlags::queenPromotion:
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece) + 4] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece + 4;
            break;
        case MoveFlags::knightPromoCapt:
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece) + 1] ^= 1ULL << move.endSquare;
            *bitboards[PieceNum2BitboardIndex.at(capturedPiece)] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece + 1;
            break;
        case MoveFlags::bishopPromoCapt:
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece) + 2] ^= 1ULL << move.endSquare;
            *bitboards[PieceNum2BitboardIndex.at(capturedPiece)] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece + 2;
            break;
        case MoveFlags::rookPromoCapt:
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece) + 3] ^= 1ULL << move.endSquare;
            *bitboards[PieceNum2BitboardIndex.at(capturedPiece)] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece + 3;
            break;
        case MoveFlags::queenPromoCapt:
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece) + 4] ^= 1ULL << move.endSquare;
            *bitboards[PieceNum2BitboardIndex.at(capturedPiece)] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece + 4;
            break;
    }

    if (movingPiece == 6) {
        legality &= ~legalityBits::whiteCanCastleMask;
    } else if (movingPiece == 14) {
        legality &= ~legalityBits::blackCanCastleMask;
    }

    if (move.startSquare == Board::Squares::a1 || move.endSquare == Board::Squares::a1) legality &= ~legalityBits::whiteLongCastleMask;
    if (move.startSquare == Board::Squares::h1 || move.endSquare == Board::Squares::h1) legality &= ~legalityBits::whiteShortCastleMask;
    if (move.startSquare == Board::Squares::a8 || move.endSquare == Board::Squares::a8) legality &= ~legalityBits::blackLongCastleMask;
    if (move.startSquare == Board::Squares::h8 || move.endSquare == Board::Squares::h8) legality &= ~legalityBits::blackShortCastleMask;

    w_pieces = w_pawn | w_knight | w_bishop | w_rook | w_queen | w_king;
    b_pieces = b_pawn | b_knight | b_bishop | b_rook | b_queen | b_king;
    all_pieces = w_pieces | b_pieces;
    empty_sqs = ~all_pieces;

    gamePhase = 1 / (1 + BitUtils::countBits(MinorPieces()) + 2 * BitUtils::countBits(MajorPieces()));

    whiteToMove = !whiteToMove;
}

void Gamestate::UndoMove() {
    Move move = moveLog.top();

    int movingPiece = mailbox[move.endSquare];
    int capturedPiece = (legality & legalityBits::capturedPieceMask) >> legalityBits::capturedPieceShift;
    U64 moveSquares = (1ULL << move.startSquare | 1ULL << move.endSquare);

    switch (move.moveFlag) {
        case MoveFlags::nullMove:
            return;
        case MoveFlags::quietMove:
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= moveSquares;
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = 0;
            break;
        case MoveFlags::doublePawnPush:
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= moveSquares;
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = 0;
            break;
        case MoveFlags::shortCastle:
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= moveSquares;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece) - 2] ^= (1ULL << (move.startSquare + 1) | 1ULL << (move.endSquare + 1));
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = 0;
            mailbox[move.startSquare + 1] = 0;
            mailbox[move.endSquare + 1] = movingPiece - 2;
            break;
        case MoveFlags::longCastle:
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= moveSquares;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece) - 2] ^= (1ULL << (move.startSquare - 1) | 1ULL << (move.endSquare - 2));
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = 0;
            mailbox[move.startSquare - 1] = 0;
            mailbox[move.endSquare - 2] = movingPiece - 2;
            break;
        case MoveFlags::capture:
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= moveSquares;
            *bitboards[PieceNum2BitboardIndex.at(capturedPiece)] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = capturedPiece;
            break;
        case MoveFlags::enPassant:
            int startRow, endCol, capturedSquare;
            startRow = move.startSquare / 8;
            endCol = move.endSquare % 8;
            capturedSquare = 8 * startRow + endCol;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= moveSquares;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece ^ 0b1000)] ^= 1ULL << capturedSquare;
            mailbox[move.startSquare] = movingPiece;
            mailbox[capturedSquare] = movingPiece ^ 0b1000;
            mailbox[move.endSquare] = 0;
            break;
        case MoveFlags::knightPromotion:
            movingPiece -= 1;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece) + 1] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = 0;
            break;
        case MoveFlags::bishopPromotion:
            movingPiece -= 2;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece) + 2] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = 0;
            break;
        case MoveFlags::rookPromotion:
            movingPiece -= 3;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece) + 3] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = 0;
            break;
        case MoveFlags::queenPromotion:
            movingPiece -= 4;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece) + 4] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = 0;
            break;
        case MoveFlags::knightPromoCapt:
            movingPiece -= 1;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece) + 1] ^= 1ULL << move.endSquare;
            *bitboards[PieceNum2BitboardIndex.at(capturedPiece)] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = capturedPiece;
            break;
        case MoveFlags::bishopPromoCapt:
            movingPiece -= 2;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece) + 2] ^= 1ULL << move.endSquare;
            *bitboards[PieceNum2BitboardIndex.at(capturedPiece)] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = capturedPiece;
            break;
        case MoveFlags::rookPromoCapt:
            movingPiece -= 3;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece) + 3] ^= 1ULL << move.endSquare;
            *bitboards[PieceNum2BitboardIndex.at(capturedPiece)] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = capturedPiece;
            break;
        case MoveFlags::queenPromoCapt:
            movingPiece -= 4;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PieceNum2BitboardIndex.at(movingPiece) + 4] ^= 1ULL << move.endSquare;
            *bitboards[PieceNum2BitboardIndex.at(capturedPiece)] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = capturedPiece;
            break;
    }

    legality = legalityHistory.top();
    moveLog.pop();
    legalityHistory.pop();

    w_pieces = w_pawn | w_knight | w_bishop | w_rook | w_queen | w_king;
    b_pieces = b_pawn | b_knight | b_bishop | b_rook | b_queen | b_king;
    all_pieces = w_pieces | b_pieces;
    empty_sqs = ~all_pieces;

    gamePhase = 1 / (1 + BitUtils::countBits(MinorPieces()) + 2 * BitUtils::countBits(MajorPieces()));

    whiteToMove = !whiteToMove;
}
