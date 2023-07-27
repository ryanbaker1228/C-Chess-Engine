//
// Created by Ryan Baker on 6/13/23.
//
#include "gamestate.h"
#include "move.h"
#include <iostream>


GAMESTATE::GAMESTATE(std::string startingPosition) {
    loadFENString(startingPosition);
    loadBitboards();
    move_log.reserve(200);
}

void GAMESTATE::loadFENString(std::string position) {
    int row = 7;
    int col = 0;

    for (char c : position) {
        if (c == '/') {
            --row;
            col = 0;
            continue;
        }
        if (c == ' ') {
            break;
        }
        if (isnumber(c)) {
            for (int i = 0; i < c - '0'; ++i) {
                mailbox[8 * row + col] = 0;
                ++col;
            }
            continue;
        }
        switch (c) {
            case 'P': mailbox[8 * row + col] = 1; break;
            case 'N': mailbox[8 * row + col] = 2; break;
            case 'B': mailbox[8 * row + col] = 3; break;
            case 'R': mailbox[8 * row + col] = 4; break;
            case 'Q': mailbox[8 * row + col] = 5; break;
            case 'K': mailbox[8 * row + col] = 6; break;
            case 'p': mailbox[8 * row + col] = 9; break;
            case 'n': mailbox[8 * row + col] = 10; break;
            case 'b': mailbox[8 * row + col] = 11; break;
            case 'r': mailbox[8 * row + col] = 12; break;
            case 'q': mailbox[8 * row + col] = 13; break;
            case 'k': mailbox[8 * row + col] = 14; break;
            default: break;
        }
        ++col;
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
    switch (move.flags) {
        case MoveFlags::nullMove:
            return;
        case MoveFlags::quietMove:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= move.sqs;
            mailbox[move.start_sq] = 0;
            mailbox[move.end_sq] = move.moving_piece;
            break;
        case MoveFlags::doublePawnPush:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= move.sqs;
            mailbox[move.start_sq] = 0;
            mailbox[move.end_sq] = move.moving_piece;
            break;
        case MoveFlags::shortCastle:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= move.sqs;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece) - 2] ^= (1ULL << (move.start_sq + 1) | 1ULL << (move.end_sq + 1));
            mailbox[move.start_sq] = 0;
            mailbox[move.end_sq] = move.moving_piece;
            mailbox[move.start_sq + 1] = move.moving_piece - 2;
            mailbox[move.end_sq + 1] = 0;
            break;
        case MoveFlags::longCastle:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= move.sqs;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece) - 2] ^= (1ULL << (move.start_sq - 1) | 1ULL << (move.end_sq - 2));
            mailbox[move.start_sq] = 0;
            mailbox[move.end_sq] = move.moving_piece;
            mailbox[move.start_sq - 1] = move.moving_piece - 2;
            mailbox[move.end_sq - 2] = 0;
            break;
        case MoveFlags::capture:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= move.sqs;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.captured_piece)] ^= 1ULL << move.end_sq;
            mailbox[move.start_sq] = 0;
            mailbox[move.end_sq] = move.moving_piece;
            break;
        case MoveFlags::enPassant:
            int startRow, endCol, capturedSquare;
            startRow = move.start_sq / 8;
            endCol = move.end_sq % 8;
            capturedSquare = 8 * startRow + endCol;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= move.sqs;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(mailbox[capturedSquare])] ^= 1ULL << capturedSquare;
            mailbox[move.start_sq] = mailbox[capturedSquare] = 0;
            mailbox[move.end_sq] = move.moving_piece;
            break;
        case MoveFlags::knightPromotion:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= 1ULL << move.start_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece) + 1] ^= 1ULL << move.end_sq;
            mailbox[move.start_sq] = 0;
            mailbox[move.end_sq] = move.moving_piece + 1;
            break;
        case MoveFlags::bishopPromotion:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= 1ULL << move.start_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece) + 2] ^= 1ULL << move.end_sq;
            mailbox[move.start_sq] = 0;
            mailbox[move.end_sq] = move.moving_piece + 2;
            break;
        case MoveFlags::rookPromotion:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= 1ULL << move.start_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece) + 3] ^= 1ULL << move.end_sq;
            mailbox[move.start_sq] = 0;
            mailbox[move.end_sq] = move.moving_piece + 3;
            break;
        case MoveFlags::queenPromotion:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= 1ULL << move.start_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece) + 4] ^= 1ULL << move.end_sq;
            mailbox[move.start_sq] = 0;
            mailbox[move.end_sq] = move.moving_piece + 4;
            break;
        case MoveFlags::knightPromoCapt:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= 1ULL << move.start_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece) + 1] ^= 1ULL << move.end_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.captured_piece)] ^= 1ULL << move.end_sq;
            mailbox[move.start_sq] = 0;
            mailbox[move.end_sq] = move.moving_piece + 1;
            break;
        case MoveFlags::bishopPromoCapt:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= 1ULL << move.start_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece) + 2] ^= 1ULL << move.end_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.captured_piece)] ^= 1ULL << move.end_sq;
            mailbox[move.start_sq] = 0;
            mailbox[move.end_sq] = move.moving_piece + 2;
            break;
        case MoveFlags::rookPromoCapt:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= 1ULL << move.start_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece) + 3] ^= 1ULL << move.end_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.captured_piece)] ^= 1ULL << move.end_sq;
            mailbox[move.start_sq] = 0;
            mailbox[move.end_sq] = move.moving_piece + 3;
            break;
        case MoveFlags::queenPromoCapt:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= 1ULL << move.start_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece) + 4] ^= 1ULL << move.end_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.captured_piece)] ^= 1ULL << move.end_sq;
            mailbox[move.start_sq] = 0;
            mailbox[move.end_sq] = move.moving_piece + 4;
            break;
    }

    w_pieces = w_pawn | w_knight | w_bishop | w_rook | w_queen | w_king;
    b_pieces = b_pawn | b_knight | b_bishop | b_rook | b_queen | b_king;
    all_pieces = w_pieces | b_pieces;
    empty_sqs = ~all_pieces;

    move_log.push_back(move);

    player_to_move = 1 - player_to_move;
}

void GAMESTATE::undoMove() {
    struct Move move = move_log.back();

    switch (move.flags) {
        case MoveFlags::nullMove:
            return;
        case MoveFlags::quietMove:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= move.sqs;
            mailbox[move.start_sq] = move.moving_piece;
            mailbox[move.end_sq] = 0;
            break;
        case MoveFlags::doublePawnPush:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= move.sqs;
            mailbox[move.start_sq] = move.moving_piece;
            mailbox[move.end_sq] = 0;
            break;
        case MoveFlags::shortCastle:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= move.sqs;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece) - 2] ^= (1ULL << (move.start_sq + 1) | 1ULL << (move.end_sq + 1));
            mailbox[move.start_sq] = move.moving_piece;
            mailbox[move.end_sq] = 0;
            mailbox[move.start_sq + 1] = 0;
            mailbox[move.end_sq + 1] = move.moving_piece - 2;
            break;
        case MoveFlags::longCastle:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= move.sqs;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece) - 2] ^= (1ULL << (move.start_sq - 1) | 1ULL << (move.end_sq - 2));
            mailbox[move.start_sq] = move.moving_piece;
            mailbox[move.end_sq] = 0;
            mailbox[move.start_sq - 1] = 0;
            mailbox[move.end_sq - 2] = move.moving_piece - 2;
            break;
        case MoveFlags::capture:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= move.sqs;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.captured_piece)] ^= 1ULL << move.end_sq;
            mailbox[move.start_sq] = move.moving_piece;
            mailbox[move.end_sq] = move.captured_piece;
            break;
        case MoveFlags::enPassant:
            int startRow, endCol, capturedSquare;
            startRow = move.start_sq / 8;
            endCol = move.end_sq % 8;
            capturedSquare = 8 * startRow + endCol;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= move.sqs;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece ^ 0b1000)] ^= 1ULL << capturedSquare;
            mailbox[move.start_sq] = move.moving_piece;
            mailbox[capturedSquare] = move.moving_piece ^ 0b1000;
            mailbox[move.end_sq] = 0;
            break;
        case MoveFlags::knightPromotion:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= 1ULL << move.start_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece) + 1] ^= 1ULL << move.end_sq;
            mailbox[move.start_sq] = move.moving_piece;
            mailbox[move.end_sq] = 0;
            break;
        case MoveFlags::bishopPromotion:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= 1ULL << move.start_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece) + 2] ^= 1ULL << move.end_sq;
            mailbox[move.start_sq] = move.moving_piece;
            mailbox[move.end_sq] = 0;
            break;
        case MoveFlags::rookPromotion:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= 1ULL << move.start_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece) + 3] ^= 1ULL << move.end_sq;
            mailbox[move.start_sq] = move.moving_piece;
            mailbox[move.end_sq] = 0;
            break;
        case MoveFlags::queenPromotion:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= 1ULL << move.start_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece) + 4] ^= 1ULL << move.end_sq;
            mailbox[move.start_sq] = move.moving_piece;
            mailbox[move.end_sq] = 0;
            break;
        case MoveFlags::knightPromoCapt:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= 1ULL << move.start_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece) + 1] ^= 1ULL << move.end_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.captured_piece)] ^= 1ULL << move.end_sq;
            mailbox[move.start_sq] = move.moving_piece;
            mailbox[move.end_sq] = move.captured_piece;
            break;
        case MoveFlags::bishopPromoCapt:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= 1ULL << move.start_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece) + 2] ^= 1ULL << move.end_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.captured_piece)] ^= 1ULL << move.end_sq;
            mailbox[move.start_sq] = move.moving_piece;
            mailbox[move.end_sq] = move.captured_piece;
            break;
        case MoveFlags::rookPromoCapt:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= 1ULL << move.start_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece) + 3] ^= 1ULL << move.end_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.captured_piece)] ^= 1ULL << move.end_sq;
            mailbox[move.start_sq] = move.moving_piece;
            mailbox[move.end_sq] = move.captured_piece;
            break;
        case MoveFlags::queenPromoCapt:
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece)] ^= 1ULL << move.start_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.moving_piece) + 4] ^= 1ULL << move.end_sq;
            *bitboards[PIECE_NUM_TO_ARRAY_INDEX.at(move.captured_piece)] ^= 1ULL << move.end_sq;
            mailbox[move.start_sq] = move.moving_piece;
            mailbox[move.end_sq] = move.captured_piece;
            break;
    }

    move_log.pop_back();

    w_pieces = w_pawn | w_knight | w_bishop | w_rook | w_queen | w_king;
    b_pieces = b_pawn | b_knight | b_bishop | b_rook | b_queen | b_king;
    all_pieces = w_pieces | b_pieces;
    empty_sqs = ~all_pieces;

    player_to_move = 1 - player_to_move;

    backup_move_log.push_back(move);
}

GAMESTATE::~GAMESTATE() = default;
