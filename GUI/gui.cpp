//
// Created by Ryan Baker on 6/12/23.
//
#include <iostream>
#include <algorithm>
#include "gui.h"
#include "../movegen.h"


GUI::GUI() {
    SDL_Init(SDL_INIT_EVERYTHING);
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        std::cout << "Failed to initialize SDL_image: " << IMG_GetError() << std::endl;
    }
    window = SDL_CreateWindow("C++ Chess Engine", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              windowWidth, windowHeight, SDL_WINDOW_SHOWN);
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
    for (auto &arrow_texture : arrow_textures) {
        SDL_DestroyTexture(arrow_texture);
    }
    std::cout << "Quitting SDL..." << std::endl;
    IMG_Quit();
    SDL_Quit();
    std::cout << "Done!" << std::endl;
}

void GUI::LoadTextures() {
    const std::string pieces[15] = {"w_pawn", "w_knight", "w_bishop", "w_rook", "w_queen", "w_king",
                                    "b_pawn", "b_knight", "b_bishop", "b_rook", "b_queen", "b_king",
                                    "empty_indicator",
                                    "full_indicator",
                                    "kill_indicator"};
    const std::string arrows[1] = {"vertical_arrow_2"};

    for (int i = 0; i < 14; ++i) {
        std::string filepath = "/Users/ryanbaker/CLionProjects/C++ Chess Engine/GUI/piece_images/" + pieces[i] + ".png";
        SDL_Surface* piece_surface = IMG_Load(filepath.c_str());
        if (piece_surface == nullptr) {
            std::cout << "Failed to load image: " << filepath << ", Error: " << IMG_GetError() << std::endl;
            continue;
        }
        piece_textures[i] = SDL_CreateTextureFromSurface(renderer, piece_surface);
        SDL_FreeSurface(piece_surface);
    }

    for (int i = 1; i <= 33; ++i) {
        std::string filepath = "/Users/ryanbaker/CLionProjects/C++ Chess Engine/GUI/arrow_sprites/arrow_" +
                std::to_string(i) + ".png";
        SDL_Surface* arrow_surface = IMG_Load(filepath.c_str());
        if (arrow_surface == nullptr) {
            std::cout << "Failed to load image: " << filepath << ", Error: " << IMG_GetError() << std::endl;
            continue;
        }
        arrow_textures[i - 1] = SDL_CreateTextureFromSurface(renderer, arrow_surface);
        SDL_FreeSurface(arrow_surface);
    }
}

void GUI::DrawGame() {
    Gamestate& gamestate = Gamestate::Get();

    SDL_RenderClear(renderer);
    DrawBoard();
    DrawPieces();
    DrawIndicators();
    DrawArrows();

    SDL_RenderPresent(renderer);
}

void GUI::DrawBoard() {
    // Draw a light square across the entire board then cover with dark squares
    SDL_Rect board = {0, 0, 8 * sqSize, 8 * sqSize };
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

        renderDestination.w = renderDestination.h = sqSize;
        if (flipBoard) {
            renderDestination.y = row * sqSize;
            renderDestination.x = (7 - col) * sqSize;
        } else {
            renderDestination.y = (7 - row) * sqSize;
            renderDestination.x = col * sqSize;
        }

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
        renderDestination.w = renderDestination.h = sqSize;
        if (flipBoard) {
            renderDestination.y = row * sqSize;
            renderDestination.x = (7 - col) * sqSize;
        } else {
            renderDestination.y = (7 - row) * sqSize;
            renderDestination.x = col * sqSize;
        }

        SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
        SDL_RenderFillRect(renderer, &renderDestination);
    }
}

void GUI::DrawPieces() {
    Gamestate& gamestate = Gamestate::Get();

    int row, col, index, sq = 0;
    for (; sq < 64; sq++) {
        if (!gamestate.mailbox[sq]) continue;
        row = sq / 8;
        col = sq % 8;
        SDL_Rect destination;
        destination.w = destination.h = sqSize;
        if (flipBoard) {
            destination.y = row * sqSize;
            destination.x = (7 - col) * sqSize;
        } else {
            destination.y = (7 - row) * sqSize;
            destination.x = col * sqSize;
        }

        index = PieceNum2BitboardIndex.at(gamestate.mailbox[sq]);
        SDL_RenderCopy(renderer, piece_textures[index], nullptr, &destination);
    }
}

void GUI::DrawIndicators() {
    Gamestate& gamestate = Gamestate::Get();

    int row, col;
    for (int sq : moveIndicatorSqs) {
        row = sq / 8;
        col = sq % 8;
        SDL_Rect destination;
        destination.w = destination.h = sqSize;
        if (flipBoard) {
            destination.y = row * sqSize;
            destination.x = (7 - col) * sqSize;
        } else {
            destination.y = (7 - row) * sqSize;
            destination.x = col * sqSize;
        }
        if (gamestate.mailbox[sq]) {
            SDL_RenderCopy(renderer, piece_textures[13], nullptr, &destination);
        } else {
            SDL_RenderCopy(renderer, piece_textures[12], nullptr, &destination);
        }
    }
}

void GUI::DrawArrows() {
    for (std::pair<int, int> arrow : drawnArrows) {
        int start_sq = arrow.first;
        int end_sq = arrow.second;

        SDL_Rect destination;
        destination.w = (1 + std::abs(start_sq % 8 - end_sq % 8)) * sqSize;
        destination.h = (1 + std::abs(start_sq / 8 - end_sq / 8)) * sqSize;
        destination.x = std::min(start_sq % 8, end_sq % 8) * sqSize;
        destination.y = (7 - std::max(start_sq / 8, end_sq / 8)) * sqSize;

        std::string hash_key = "{" + std::to_string(end_sq / 8 - start_sq / 8) + ", " + std::to_string(end_sq % 8 - start_sq % 8) + "}";

        if (!arrow_map.contains(hash_key)) {
            drawnArrows.erase(std::find(drawnArrows.begin(), drawnArrows.end(), arrow));
            continue;
        }

        std::pair<int, SDL_RendererFlip> arrow_info = arrow_map.at(hash_key);
        SDL_RenderCopyEx(renderer, arrow_textures[arrow_info.first], nullptr, &destination, 0.0, nullptr, arrow_info.second);
    }
}

void GUI::HandleButtonClick(SDL_MouseButtonEvent event) {
    int mouseX, mouseY, mouseSquare;
    SDL_GetMouseState(&mouseX, &mouseY);

    int selectedSquare;
    if (flipBoard) {
        selectedSquare = 8 * (mouseY / sqSize) + (7 - mouseX / sqSize);
    } else {
        selectedSquare = 8 * (7 - mouseY / sqSize) + (mouseX / sqSize);
    }

    if (event.button == SDL_BUTTON_RIGHT) {
        if (event.type == SDL_MOUSEBUTTONDOWN) {
            arrow_start_square = selectedSquare;
        } else if (event.type == SDL_MOUSEBUTTONUP) {
            std::pair<int, int> arrow = {arrow_start_square, selectedSquare};

            if (std::find(drawnArrows.begin(), drawnArrows.end(), arrow) != drawnArrows.end()) {
                drawnArrows.erase(std::find(drawnArrows.begin(), drawnArrows.end(), arrow));
            } else {
                drawnArrows.push_back({arrow_start_square, selectedSquare});
            }
        }
        return;
    }

    if (event.type == SDL_MOUSEBUTTONUP) return;

    Gamestate& gamestate = Gamestate::Get();

    MoveGenerator::Get().GenerateLegalMoves();

    drawnArrows.clear();
    if ((gamestate.empty_sqs & 1ULL << selectedSquare) && selectedSqs.empty()) {
        /* The user has selected an empty square */
        selectedSqs.clear();
        UpdateHighlights();
        return;
    } else {
        /* The user has selected a piece */
        selectedSqs.push_back(selectedSquare);
        UpdateHighlights();
        highlightedSqs.push_back(selectedSquare);
    }

    if (selectedSqs.size() == 1) {
        /* The user selected a piece, check if the piece has any moves */
        moveIndicatorSqs.clear();
        for (struct Move move: MoveGenerator::Get().GenerateLegalMoves()) {
            if (move.startSquare == selectedSquare) {
                moveIndicatorSqs.push_back(move.endSquare);
            }
        }
        if (moveIndicatorSqs.empty()) {
            /* The user has selected a piece that cannot move */
            UpdateHighlights();
            highlightedSqs.push_back(selectedSquare);
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
            UpdateHighlights();
            return;
        }

        for (struct Move move: MoveGenerator::Get().GenerateLegalMoves()) {
            if (move.startSquare == selectedSqs[0] && move.endSquare == selectedSqs[1]) {
                if (move.flag & MoveFlags::promotion) {
                    int promotionPiece = PollPromotion(move.endSquare);
                    switch (promotionPiece) {
                        case 2:
                        case 10:
                            move.flag |= MoveFlags::knightPromotion;
                            break;
                        case 3:
                        case 11:
                            move.flag |= MoveFlags::bishopPromotion;
                            break;
                        case 4:
                        case 12:
                            move.flag |= MoveFlags::rookPromotion;
                            break;
                        case 5:
                        case 13:
                            move.flag |= MoveFlags::queenPromotion;
                            break;
                        default:
                            UpdateHighlights();
                            selectedSqs.clear();
                            moveIndicatorSqs.clear();
                            return;
                    }
                }
                gamestate.MakeMove(move);
                highlightedSqs = {move.startSquare, move.endSquare};
                selectedSqs.clear();
                moveIndicatorSqs.clear();
                return;
            }
        }
        moveIndicatorSqs.clear();
        selectedSqs = {selectedSquare};

        for (struct Move move: MoveGenerator::Get().GenerateLegalMoves()) {
            if (move.startSquare == selectedSquare) {
                moveIndicatorSqs.push_back(move.endSquare);
            }
        }
        if (moveIndicatorSqs.empty() && gamestate.empty_sqs & 1ULL << selectedSquare) {
            selectedSqs.clear();
        }

        auto uniqueIndicators = std::unique(moveIndicatorSqs.begin(), moveIndicatorSqs.end());
        moveIndicatorSqs.resize(std::distance(moveIndicatorSqs.begin(), uniqueIndicators));
        UpdateHighlights();
        if (!(gamestate.empty_sqs & 1ULL << selectedSquare)) highlightedSqs.push_back(selectedSquare);
    }
}

void GUI::HandleKeyPress(SDL_Keycode key) {
    Gamestate& gamestate = Gamestate::Get();

    switch (key) {
        case SDLK_LEFT:
            if (!gamestate.moveLog.empty()) {
                gamestate.UndoMove();
                UpdateHighlights();
                moveIndicatorSqs.clear();
                selectedSqs.clear();
            }
            break;

        case SDLK_RIGHT:
            if (!backupMoveLog.empty()) {
                gamestate.MakeMove(backupMoveLog.top());
                backupMoveLog.pop();
                highlightedSqs = {gamestate.moveLog.top().startSquare, gamestate.moveLog.top().endSquare};
                moveIndicatorSqs.clear();
                selectedSqs.clear();
            }
            break;

        case SDLK_f:
            GUI::Get().flipBoard = !GUI::Get().flipBoard;

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

    dimmingMask.h = dimmingMask.w = 8 * sqSize;
    dimmingMask.x = dimmingMask.y = 0;
    SDL_SetRenderDrawColor(renderer, 43, 43, 43, 200);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_RenderFillRect(renderer, &dimmingMask);

    if (row == 7) {
        promotingPiece = 1;
        queenDestination = {col * sqSize, sqSize * 0, sqSize, sqSize};
        knightDestination = {col * sqSize, sqSize * 1, sqSize, sqSize};
        rookDestination = {col * sqSize, sqSize * 2, sqSize, sqSize};
        bishopDestination = {col * sqSize, sqSize * 3, sqSize, sqSize};
        queenDestSquare = promotionSquare - 8 * 0;
        knightDestSquare = promotionSquare - 8 * 1;
        rookDestSquare = promotionSquare - 8 * 2;
        bishopDestSquare = promotionSquare - 8 * 3;
    } else {
        promotingPiece = 9;
        queenDestination = {col * sqSize, sqSize * 7, sqSize, sqSize};
        knightDestination = {col * sqSize, sqSize * 6, sqSize, sqSize};
        rookDestination = {col * sqSize, sqSize * 5, sqSize, sqSize};
        bishopDestination = {col * sqSize, sqSize * 4, sqSize, sqSize};
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

    SDL_RenderCopy(renderer, piece_textures[PieceNum2BitboardIndex.at(promotingPiece) + 4], nullptr, &queenDestination);
    SDL_RenderCopy(renderer, piece_textures[PieceNum2BitboardIndex.at(promotingPiece) + 1], nullptr, &knightDestination);
    SDL_RenderCopy(renderer, piece_textures[PieceNum2BitboardIndex.at(promotingPiece) + 3], nullptr, &rookDestination);
    SDL_RenderCopy(renderer, piece_textures[PieceNum2BitboardIndex.at(promotingPiece) + 2], nullptr, &bishopDestination);

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
                    mouseSquare = 8 * (7 - mouseY / sqSize) + mouseX / sqSize;
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
/*
void GUI::DrawArrow(int start_sq, int end_sq) {

    int arrowhead_size = 30;
    int line_thickness = 10;
    float arrowhead_sharpness = 20 * M_PI / 180;

    int startX = (start_sq % 8) * sqSize + sqSize / 2;
    int startY = (7 - (start_sq / 8)) * sqSize + sqSize / 2;
    int endX = (end_sq % 8) * sqSize + sqSize / 2;
    int endY = (7 - (end_sq / 8)) * sqSize + sqSize / 2;

    SDL_SetRenderDrawColor(renderer, 99, 175, 70, 255);  // Red color

    // Calculate the angle of the arrow
    double angle = atan2(endY - startY, endX - startX);

    // Calculate the arrowhead points
    int arrowX1 = endX - arrowhead_size * cos(angle - M_PI / 6);
    int arrowY1 = endY - arrowhead_size * sin(angle - M_PI / 6);

    int arrowX2 = endX;
    int arrowY2 = endY;

    int arrowX3 = endX - arrowhead_size * cos(angle + M_PI / 6);
    int arrowY3 = endY - arrowhead_size * sin(angle + M_PI / 6);

    // Draw the arrow line as multiple filled rectangles to create thickness
    int numLines = line_thickness;
    int lineSpacing = line_thickness - 1;  // Adjust for the spacing between lines

    int offsetX, offsetY;
    for (int i = 0; i < numLines; ++i) {
        offsetX = ((i - (numLines - 1) / 2) * lineSpacing * -sin(angle)) / 10;
        offsetY = ((i - (numLines - 1) / 2) * lineSpacing * -cos(angle)) / 10;

        // Calculate the coordinates of the current line
        int lineStartX = startX + offsetX;
        int lineStartY = startY + offsetY;
        int lineEndX = endX + offsetX;
        int lineEndY = endY + offsetY;

        SDL_RenderDrawLine(renderer, lineStartX, lineStartY, lineEndX, lineEndY);
    }

    // Draw the arrowhead as a triangle
    SDL_RenderDrawLine(renderer, arrowX1, arrowY1, arrowX2, arrowY2);
    SDL_RenderDrawLine(renderer, arrowX2, arrowY2, arrowX3, arrowY3);
    SDL_RenderDrawLine(renderer, arrowX3, arrowY3, arrowX1, arrowY1);
}

    float angle = atan2(endY - startY, endX - startX);
    float side_len = (arrowhead_size / 2) / cos(arrowhead_sharpness);

    Point point_1 = {endX, endY};
    Point point_2 = {endX + int(cos(M_PI + angle - arrowhead_sharpness) * arrowhead_size),
                     endY + int(sin(M_PI + angle - arrowhead_sharpness) * arrowhead_size)};
    Point point_3 = {endX + int(-cos(angle) * side_len), endY + int(-sin(angle) * side_len)};
    Point point_4 = {endX + int(cos(M_PI + angle + arrowhead_sharpness) * arrowhead_size),
                     endY + int(sin(M_PI + angle + arrowhead_sharpness) * arrowhead_size)};

    PolygonShape arrowhead_1 = {{point_1, point_2, point_3}};
    PolygonShape arrowhead_2 = {{point_1, point_3, point_4}};

    DrawFilledPolygon(arrowhead_1, BoardThemes::blueTheme.lightHighlight, renderer);
    DrawFilledPolygon(arrowhead_2, BoardThemes::blueTheme.lightHighlight, renderer);

    return;
}
*/

void GUI::UpdateHighlights() {
    if (Gamestate::Get().moveLog.empty()) {
        highlightedSqs.clear();
        return;
    }
    highlightedSqs = {Gamestate::Get().moveLog.top().startSquare, Gamestate::Get().moveLog.top().endSquare};
    drawnArrows.clear();
}
