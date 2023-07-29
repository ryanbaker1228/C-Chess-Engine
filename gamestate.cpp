//
// Created by Ryan Baker on 6/13/23.
//
#include "gamestate.h"
#include "move.h"
#include "movegen.h"
#include <iostream>


GAMESTATE::GAMESTATE(const std::string& startingPosition) {
    loadFENString(startingPosition);
    loadBitboards();
    move_log.reserve(200);
}

void GAMESTATE::loadFENString(const std::string& position) {
    enum FENStringFields {piecePlacement, activeColor, castlingRights, enPassantTargets, halfMoveClock, fullMoveCount};
    int currentField = piecePlacement;
    int row = 7;
    int col = 0;

    for (char c : position) {
        switch (currentField) {
            case piecePlacement:
                if (c == '/') {
                    --row;
                    col = 0;
                    continue;
                }
                if (isnumber(c)) {
                    for (int i = 0; i < c - '0'; ++i) {
                        mailbox[8 * row + col] = 0;
                        ++col;
                    }
                    continue;
                }
                switch (c) {
                    case 'P':
                        mailbox[8 * row + col] = 1;
                        break;
                    case 'N':
                        mailbox[8 * row + col] = 2;
                        break;
                    case 'B':
                        mailbox[8 * row + col] = 3;
                        break;
                    case 'R':
                        mailbox[8 * row + col] = 4;
                        break;
                    case 'Q':
                        mailbox[8 * row + col] = 5;
                        break;
                    case 'K':
                        mailbox[8 * row + col] = 6;
                        break;
                    case 'p':
                        mailbox[8 * row + col] = 9;
                        break;
                    case 'n':
                        mailbox[8 * row + col] = 10;
                        break;
                    case 'b':
                        mailbox[8 * row + col] = 11;
                        break;
                    case 'r':
                        mailbox[8 * row + col] = 12;
                        break;
                    case 'q':
                        mailbox[8 * row + col] = 13;
                        break;
                    case 'k':
                        mailbox[8 * row + col] = 14;
                        break;
                    case ' ': default:
                        ++currentField;
                        break;
                }
                ++col;
                break;

            case activeColor:
                if (c == ' ') {
                    ++currentField;
                    break;
                }
                whiteToMove = c == 'w';
                break;

            case castlingRights:
                switch (c) {
                    case 'K':
                        legality |= 1 << legalityBits::whiteShortCastleShift;
                        break;
                    case 'Q':
                        legality |= 1 << legalityBits::whiteLongCastleShift;
                        break;
                    case 'k':
                        legality |= 1 << legalityBits::blackShortCastleShift;
                        break;
                    case 'q':
                        legality |= 1 << legalityBits::blackLongCastleShift;
                        break;
                    case ' ': default:
                        ++currentField;
                        break;
                }
                break;

            case enPassantTargets:
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
                } else {
                    col = c - 'a';
                }

            case halfMoveClock:
                if (c == ' ') {
                    ++currentField;
                    break;
                }
                plyCount = plyCount * 10 + (c - ' ');
                break;

            case fullMoveCount:
            default:
                break;
        }
    }
}

void GAMESTATE::loadBitboards() {
    for (int sq = 0; sq < 64; ++sq) {
        if (!mailbox[sq]) {
            continue;
        }
        *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(mailbox[sq])] |= 1ULL << sq;
    }
    w_pieces = w_pawn | w_knight | w_bishop | w_rook | w_queen | w_king;
    b_pieces = b_pawn | b_knight | b_bishop | b_rook | b_queen | b_king;
    all_pieces = w_pieces | b_pieces;
    empty_sqs = ~all_pieces;
}

void GAMESTATE::makeMove(Move move) {
    legalityHistory.push(legality);
    legality = legalityHistory.top() & legalityBits::castleMask;

    int movingPiece = mailbox[move.startSquare];
    int capturedPiece = mailbox[move.endSquare];
    U64 moveSquares = (1ULL << move.startSquare | 1ULL << move.endSquare);

    switch (move.moveFlag) {
        case MoveFlags::nullMove:
            return;
        case MoveFlags::quietMove:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= moveSquares;
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece;
            break;
        case MoveFlags::doublePawnPush:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= moveSquares;
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece;
            legality |= legalityBits::enPassantLegalMask;
            legality |= (move.endSquare & 8) << legalityBits::enPassantFileShift;
            break;
        case MoveFlags::shortCastle:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= moveSquares;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece) - 2] ^= (1ULL << (move.startSquare + 1) | 1ULL << (move.endSquare + 1));
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece;
            mailbox[move.startSquare + 1] = movingPiece - 2;
            mailbox[move.endSquare + 1] = 0;
            break;
        case MoveFlags::longCastle:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= moveSquares;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece) - 2] ^= (1ULL << (move.startSquare - 1) | 1ULL << (move.endSquare - 2));
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece;
            mailbox[move.startSquare - 1] = movingPiece - 2;
            mailbox[move.endSquare - 2] = 0;
            break;
        case MoveFlags::capture:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= moveSquares;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(capturedPiece)] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece;
            break;
        case MoveFlags::enPassant:
            int startRow, endCol, capturedSquare;
            startRow = move.startSquare / 8;
            endCol = move.endSquare % 8;
            capturedSquare = 8 * startRow + endCol;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= moveSquares;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(mailbox[capturedSquare])] ^= 1ULL << capturedSquare;
            mailbox[move.startSquare] = mailbox[capturedSquare] = 0;
            mailbox[move.endSquare] = movingPiece;
            break;
        case MoveFlags::knightPromotion:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece) + 1] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece + 1;
            break;
        case MoveFlags::bishopPromotion:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece) + 2] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece + 2;
            break;
        case MoveFlags::rookPromotion:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece) + 3] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece + 3;
            break;
        case MoveFlags::queenPromotion:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece) + 4] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece + 4;
            break;
        case MoveFlags::knightPromoCapt:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece) + 1] ^= 1ULL << move.endSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(capturedPiece)] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece + 1;
            break;
        case MoveFlags::bishopPromoCapt:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece) + 2] ^= 1ULL << move.endSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(capturedPiece)] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece + 2;
            break;
        case MoveFlags::rookPromoCapt:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece) + 3] ^= 1ULL << move.endSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(capturedPiece)] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece + 3;
            break;
        case MoveFlags::queenPromoCapt:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece) + 4] ^= 1ULL << move.endSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(capturedPiece)] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = 0;
            mailbox[move.endSquare] = movingPiece + 4;
            break;
    }

    if (movingPiece == 6) {
        legality &= ~legalityBits::whiteCanCastleMask;
    } else if (movingPiece == 14) {
        legality &= ~legalityBits::blackCanCastleMask;
    }

    w_pieces = w_pawn | w_knight | w_bishop | w_rook | w_queen | w_king;
    b_pieces = b_pawn | b_knight | b_bishop | b_rook | b_queen | b_king;
    all_pieces = w_pieces | b_pieces;
    empty_sqs = ~all_pieces;

    move_log.push_back(move);

    legality |= capturedPiece << legalityBits::capturedPieceShift;

    whiteToMove = !whiteToMove;
}

void GAMESTATE::undoMove() {
    struct Move move = move_log.back();

    int movingPiece = mailbox[move.endSquare];
    int capturedPiece = (legality & legalityBits::capturedPieceMask) >> legalityBits::capturedPieceShift;
    U64 moveSquares = (1ULL << move.startSquare | 1ULL << move.endSquare);

    switch (move.moveFlag) {
        case MoveFlags::nullMove:
            return;
        case MoveFlags::quietMove:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= moveSquares;
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = 0;
            break;
        case MoveFlags::doublePawnPush:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= moveSquares;
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = 0;
            break;
        case MoveFlags::shortCastle:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= moveSquares;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece) - 2] ^= (1ULL << (move.startSquare + 1) | 1ULL << (move.endSquare + 1));
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = 0;
            mailbox[move.startSquare + 1] = 0;
            mailbox[move.endSquare + 1] = movingPiece - 2;
            break;
        case MoveFlags::longCastle:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= moveSquares;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece) - 2] ^= (1ULL << (move.startSquare - 1) | 1ULL << (move.endSquare - 2));
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = 0;
            mailbox[move.startSquare - 1] = 0;
            mailbox[move.endSquare - 2] = movingPiece - 2;
            break;
        case MoveFlags::capture:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= moveSquares;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(capturedPiece)] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = capturedPiece;
            break;
        case MoveFlags::enPassant:
            int startRow, endCol, capturedSquare;
            startRow = move.startSquare / 8;
            endCol = move.endSquare % 8;
            capturedSquare = 8 * startRow + endCol;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= moveSquares;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece ^ 0b1000)] ^= 1ULL << capturedSquare;
            mailbox[move.startSquare] = movingPiece;
            mailbox[capturedSquare] = movingPiece ^ 0b1000;
            mailbox[move.endSquare] = 0;
            break;
        case MoveFlags::knightPromotion:
            movingPiece -= 1;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece) + 1] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = 0;
            break;
        case MoveFlags::bishopPromotion:
            movingPiece -= 2;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece) + 2] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = 0;
            break;
        case MoveFlags::rookPromotion:
            movingPiece -= 3;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece) + 3] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = 0;
            break;
        case MoveFlags::queenPromotion:
            movingPiece -= 4;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece) + 4] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = 0;
            break;
        case MoveFlags::knightPromoCapt:
            movingPiece -= 1;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece) + 1] ^= 1ULL << move.endSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(capturedPiece)] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = capturedPiece;
            break;
        case MoveFlags::bishopPromoCapt:
            movingPiece -= 2;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece) + 2] ^= 1ULL << move.endSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(capturedPiece)] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = capturedPiece;
            break;
        case MoveFlags::rookPromoCapt:
            movingPiece -= 3;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece) + 3] ^= 1ULL << move.endSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(capturedPiece)] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = capturedPiece;
            break;
        case MoveFlags::queenPromoCapt:
            movingPiece -= 4;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece)] ^= 1ULL << move.startSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(movingPiece) + 4] ^= 1ULL << move.endSquare;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(capturedPiece)] ^= 1ULL << move.endSquare;
            mailbox[move.startSquare] = movingPiece;
            mailbox[move.endSquare] = capturedPiece;
            break;
    }

    move_log.pop_back();
    legality = legalityHistory.top();
    legalityHistory.pop();

    w_pieces = w_pawn | w_knight | w_bishop | w_rook | w_queen | w_king;
    b_pieces = b_pawn | b_knight | b_bishop | b_rook | b_queen | b_king;
    all_pieces = w_pieces | b_pieces;
    empty_sqs = ~all_pieces;

    whiteToMove = !whiteToMove;

    backup_move_log.push_back(move);
}

GAMESTATE::~GAMESTATE() = default;
