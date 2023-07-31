//
// Created by Ryan Baker on 6/12/23.
//
#include <iostream>
#include <algorithm>
#include "gui.h"
#include "move.h"
#include "movegen.h"


GUI::GUI() {
    SDL_Init(SDL_INIT_EVERYTHING);
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cout << "Failed to initialize SDL_image: " << IMG_GetError() << std::endl;
    }
    window = SDL_CreateWindow("C++ Chess Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC);
    LoadTextures();
}

GUI::~GUI() {
    std::cout << "Erasing Vectors..." << std::endl;
    std::destroy(highlightedSqs.begin(), highlightedSqs.end());
    std::destroy(selectedSqs.begin(), selectedSqs.end());
    std::cout << "Destroying Renderer..." << std::endl;
    SDL_DestroyRenderer(GUI::renderer);
    SDL_DestroyWindow(GUI::window);
    std::cout << "Destroying Textures..." << std::endl;
    for (auto &piece_texture : piece_textures) {
        SDL_DestroyTexture(piece_texture);
    }
    std::cout << "Quitting SDL..." << std::endl;
    IMG_Quit();
    SDL_Quit();
    std::cout << "Done!" << std::endl;
}

void GUI::LoadTextures() {
    const std::string pieces[14] = {"w_pawn", "w_knight", "w_bishop", "w_rook", "w_queen", "w_king",
                                    "b_pawn", "b_knight", "b_bishop", "b_rook", "b_queen", "b_king",
                                    "empty_indicator", // legal move to an empty sq
                                    "full_indicator"}; // legal move to an occupied square
    for (int i = 0; i < 14; ++i) {
        std::string filepath = "../piece_images/" + pieces[i] + ".png";
        SDL_Surface* piece_surface = IMG_Load(filepath.c_str());
        if (piece_surface == nullptr) {
            std::cout << "Failed to load image: " << filepath << ", Error: " << IMG_GetError() << std::endl;
            continue;  // Skip to the next iteration if loading fails
        }
        piece_textures[i] = SDL_CreateTextureFromSurface(renderer, piece_surface);
        SDL_FreeSurface(piece_surface);
    }

}

void GUI::DrawGame(GAMESTATE* gamestate) {
    SDL_RenderClear(renderer);
    DrawBoard();
    DrawPieces(gamestate);
    DrawIndicators(gamestate);
    SDL_RenderPresent(renderer);
}

void GUI::DrawBoard() {
    /* Draw a light square across the entire board then cover with dark squares */
    SDL_Rect board = { 0, 0, 8 * SQ_SIZE, 8 * SQ_SIZE };
    SDL_Color color = theme.lightSquareColor;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &board);

    SDL_Rect renderDestination;
    color = theme.darkSquareColor;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    int row, col;
    for (int square = 0; square < 64; ++square) {
        row = square / 8;
        col = square % 8;
        if ((row + col) % 2) continue;

        renderDestination.w = renderDestination.h = SQ_SIZE;
        renderDestination.y = (7 - row) * SQ_SIZE;
        renderDestination.x = col * SQ_SIZE;

        SDL_RenderFillRect(renderer, &renderDestination);
    }

    for (int square : highlightedSqs) {
        row = square / 8;
        col = square % 8;

        if ((row + col) % 2) {
            color = theme.lightHighlight;
        } else {
            color = theme.darkHighlight;
        }
        renderDestination.w = renderDestination.h = SQ_SIZE;
        renderDestination.y = (7 - row) * SQ_SIZE;
        renderDestination.x = col * SQ_SIZE;

        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &renderDestination);
    }
}

void GUI::DrawPieces(GAMESTATE* gamestate) {
    int row, col, index, sq = 0;
    for (; sq < 64; sq++) {
        if (!gamestate->mailbox[sq]) continue;
        row = sq / 8;
        col = sq % 8;
        SDL_Rect destination;
        destination.w = destination.h = SQ_SIZE;
        destination.y = (7 - row) * SQ_SIZE;
        destination.x = col * SQ_SIZE;
        index = PIECE_NUM_TO_IMAGE_ARRAY_INDEX.at(gamestate->mailbox[sq]);
        SDL_RenderCopy(renderer, piece_textures[index], nullptr, &destination);
    }
}

void GUI::DrawIndicators(GAMESTATE* gamestate) {
    int row, col;
    for (int sq : moveIndicatorSqs) {
        row = sq / 8;
        col = sq % 8;
        SDL_Rect destination;
        destination.w = destination.h = SQ_SIZE;
        destination.y = (7 - row) * SQ_SIZE;
        destination.x = col * SQ_SIZE;
        if (gamestate->mailbox[sq]) {
            SDL_RenderCopy(renderer, piece_textures[13], nullptr, &destination);
        } else {
            SDL_RenderCopy(renderer, piece_textures[12], nullptr, &destination);
        }
    }
}

void GUI::HandleButtonClick(SDL_MouseButtonEvent event, GAMESTATE* gamestate) {
    int selectedSquare = 8 * (7 - (event.y / SQ_SIZE)) + (event.x / SQ_SIZE);
    MoveGenerator::Get()->GenerateLegalMoves();

    if ((gamestate->empty_sqs & 1ULL << selectedSquare) && selectedSqs.empty()) {
        /* The user has selected an empty square */
        selectedSqs.clear();
        if (!gamestate->move_log.empty()) {
            highlightedSqs = {gamestate->move_log.back().startSquare, gamestate->move_log.back().endSquare};
        } else {
            highlightedSqs.clear();
        }
        return;
    } else {
        /* The user has selected a piece */
        selectedSqs.push_back(selectedSquare);
        if (!gamestate->move_log.empty()) {
            highlightedSqs = {gamestate->move_log.back().startSquare, gamestate->move_log.back().endSquare, selectedSquare};
        } else {
            highlightedSqs = {selectedSquare};
        }
    }

    if (selectedSqs.size() == 1) {
        /* The user selected a piece, check if the piece has any moves */
        moveIndicatorSqs.clear();
        for (struct Move move: MoveGenerator::Get()->legalMoves) {
            if (move.startSquare == selectedSquare) {
                moveIndicatorSqs.push_back(move.endSquare);
            }
        }
        if (moveIndicatorSqs.empty()) {
            /* The user has selected a piece that cannot move */
            if (!gamestate->move_log.empty()) {
                highlightedSqs = {gamestate->move_log.back().startSquare, gamestate->move_log.back().endSquare,
                                  selectedSquare};
            } else {
                highlightedSqs = {selectedSquare};
            }
            return;
        }
        auto uniqueIndicators = std::unique(moveIndicatorSqs.begin(), moveIndicatorSqs.end());
        moveIndicatorSqs.resize(std::distance(moveIndicatorSqs.begin(), uniqueIndicators));
    }

    if (selectedSqs.size() == 2) {
        if (selectedSqs[0] == selectedSqs[1]) {
            /* User selected the same piece twice */
            selectedSqs.clear();
            moveIndicatorSqs.clear();
            if (!gamestate->move_log.empty()) {
                highlightedSqs = {gamestate->move_log.back().startSquare, gamestate->move_log.back().endSquare};
            } else {
                highlightedSqs.clear();
            }
            return;
        }

        for (struct Move move: MoveGenerator::Get()->legalMoves) {
            if (move.startSquare == selectedSqs[0] && move.endSquare == selectedSqs[1]) {
                if (move.moveFlag & MoveFlags::promotion) {
                    int promotionPiece = PollPromotion(move.endSquare);
                    switch (promotionPiece) {
                        case 2:
                        case 10:
                            move.moveFlag |= MoveFlags::knightPromotion;
                            break;
                        case 3:
                        case 11:
                            move.moveFlag |= MoveFlags::bishopPromotion;
                            break;
                        case 4:
                        case 12:
                            move.moveFlag |= MoveFlags::rookPromotion;
                            break;
                        case 5:
                        case 13:
                            move.moveFlag |= MoveFlags::queenPromotion;
                            break;
                        default:
                            if (!gamestate->move_log.empty()) {
                                highlightedSqs = {gamestate->move_log.back().startSquare,
                                                  gamestate->move_log.back().endSquare};
                            } else {
                                highlightedSqs.clear();
                            }
                            selectedSqs.clear();
                            moveIndicatorSqs.clear();
                            return;
                    }
                }
                gamestate->makeMove(move);
                gamestate->backup_move_log.clear();
                highlightedSqs = {move.startSquare, move.endSquare};
                selectedSqs.clear();
                moveIndicatorSqs.clear();
                return;
            }
        }
        moveIndicatorSqs.clear();
        selectedSqs = {selectedSquare};

        for (struct Move move: MoveGenerator::Get()->legalMoves) {
            if (move.startSquare == selectedSquare) {
                moveIndicatorSqs.push_back(move.endSquare);
            }
        }
        if (moveIndicatorSqs.empty() && gamestate->empty_sqs & 1ULL << selectedSquare) {
            selectedSqs.clear();
        }

        auto uniqueIndicators = std::unique(moveIndicatorSqs.begin(), moveIndicatorSqs.end());
        moveIndicatorSqs.resize(std::distance(moveIndicatorSqs.begin(), uniqueIndicators));
        if (!gamestate->move_log.empty()) {
            highlightedSqs = {gamestate->move_log.back().startSquare, gamestate->move_log.back().endSquare, selectedSquare};
        } else if (!(gamestate->empty_sqs & 1ULL << selectedSquare)) {
            highlightedSqs = {selectedSquare};
        } else {
            highlightedSqs.clear();
        }
    }
}

void GUI::HandleKeyPress(SDL_Keycode key, GAMESTATE* gamestate) {
    switch (key) {
        case SDLK_LEFT:
            if (!gamestate->move_log.empty()) {
                gamestate->undoMove();
                if (!gamestate->move_log.empty()) {
                    highlightedSqs = {gamestate->move_log.back().startSquare, gamestate->move_log.back().endSquare};
                } else {
                    highlightedSqs.clear();
                }
                moveIndicatorSqs.clear();
                selectedSqs.clear();
            }
            break;
        case SDLK_RIGHT:
            if (!gamestate->backup_move_log.empty()) {
                gamestate->makeMove(gamestate->backup_move_log.back());
                gamestate->backup_move_log.pop_back();
                highlightedSqs = {gamestate->move_log.back().startSquare, gamestate->move_log.back().endSquare};
                moveIndicatorSqs.clear();
                selectedSqs.clear();
            }
            break;
        default:
            break;
    }
}

int GUI::PollPromotion(int promotionSquare) {
    int row = promotionSquare / 8;
    int col = promotionSquare % 8;
    int promotingPiece, queenDestSquare, knightDestSquare, rookDestSquare, bishopDestSquare;
    SDL_Rect queenDestination, knightDestination, rookDestination, bishopDestination, dimmingMask;
    SDL_Color color;

    dimmingMask.h = dimmingMask.w = 8 * SQ_SIZE;
    dimmingMask.x = dimmingMask.y = 0;
    SDL_SetRenderDrawColor(renderer, 43, 43, 43, 200);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderFillRect(renderer, &dimmingMask);

    if (row == 7) {
        promotingPiece = 1;
        queenDestination = {col * SQ_SIZE, SQ_SIZE * 0, SQ_SIZE, SQ_SIZE};
        knightDestination = {col * SQ_SIZE, SQ_SIZE * 1, SQ_SIZE, SQ_SIZE};
        rookDestination = {col * SQ_SIZE, SQ_SIZE * 2, SQ_SIZE, SQ_SIZE};
        bishopDestination = {col * SQ_SIZE, SQ_SIZE * 3, SQ_SIZE, SQ_SIZE};
        queenDestSquare = promotionSquare - 8 * 0;
        knightDestSquare = promotionSquare - 8 * 1;
        rookDestSquare = promotionSquare - 8 * 2;
        bishopDestSquare = promotionSquare - 8 * 3;
    } else {
        promotingPiece = 9;
        queenDestination = {col * SQ_SIZE, SQ_SIZE * 7, SQ_SIZE, SQ_SIZE};
        knightDestination = {col * SQ_SIZE, SQ_SIZE * 6, SQ_SIZE, SQ_SIZE};
        rookDestination = {col * SQ_SIZE, SQ_SIZE * 5, SQ_SIZE, SQ_SIZE};
        bishopDestination = {col * SQ_SIZE, SQ_SIZE * 4, SQ_SIZE, SQ_SIZE};
        queenDestSquare = promotionSquare + 8 * 0;
        knightDestSquare = promotionSquare + 8 * 1;
        rookDestSquare = promotionSquare + 8 * 2;
        bishopDestSquare = promotionSquare + 8 * 3;
    }

    color = (row + col) % 2 ? theme.lightSquareColor : theme.darkSquareColor;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &queenDestination);
    SDL_RenderFillRect(renderer, &rookDestination);

    color = (row + col) % 2 ? theme.darkSquareColor : theme.lightSquareColor;
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &knightDestination);
    SDL_RenderFillRect(renderer, &bishopDestination);

    SDL_RenderCopy(renderer, piece_textures[PIECE_NUM_TO_IMAGE_ARRAY_INDEX.at(promotingPiece) + 4], nullptr, &queenDestination);
    SDL_RenderCopy(renderer, piece_textures[PIECE_NUM_TO_IMAGE_ARRAY_INDEX.at(promotingPiece) + 1], nullptr, &knightDestination);
    SDL_RenderCopy(renderer, piece_textures[PIECE_NUM_TO_IMAGE_ARRAY_INDEX.at(promotingPiece) + 3], nullptr, &rookDestination);
    SDL_RenderCopy(renderer, piece_textures[PIECE_NUM_TO_IMAGE_ARRAY_INDEX.at(promotingPiece) + 2], nullptr, &bishopDestination);

    SDL_RenderPresent(renderer);

    SDL_Event event;
    bool selected = false;

    while (!selected) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_MOUSEBUTTONDOWN:
                    selected = true;
                    int mouseX, mouseY, mouseSquare;
                    SDL_GetMouseState(&mouseX, &mouseY);
                    mouseSquare = 8 * (7 - mouseY / SQ_SIZE) + mouseX / SQ_SIZE;
                    if (mouseSquare == queenDestSquare) return promotingPiece + 4;
                    if (mouseSquare == knightDestSquare) return promotingPiece + 1;
                    if (mouseSquare == rookDestSquare) return promotingPiece + 3;
                    if (mouseSquare == bishopDestSquare) return promotingPiece + 2;
                    break;
            }
        }
    }
    return 0;
}
