//
// Created by Ryan Baker on 6/12/23.
//

#pragma once
#include "gamestate.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <vector>
#include <unordered_map>


namespace BoardThemes {
    struct ColorTheme {
        SDL_Color lightSquareColor;
        SDL_Color darkSquareColor;
        SDL_Color lightHighlight;
        SDL_Color darkHighlight;
    };

    inline ColorTheme brownTheme {{237, 217, 185, 255},
                                  {174, 137, 104, 255},
                                  {246, 235, 134, 255},
                                  {215, 195, 94, 255}};

    inline ColorTheme blueTheme = {{234, 232, 212, 255},
                                   {83, 115, 150, 255},
                                   {199, 215, 160, 255},
                                   {153, 176, 126, 255}};
}

class GUI {
public:
    GUI();
    ~GUI();

    void LoadTextures();
    void DrawGame(GAMESTATE* gamestate);
    void DrawBoard();
    void DrawPieces(GAMESTATE* gamestate);
    void DrawIndicators(GAMESTATE* gamestate);

    void HandleButtonClick(SDL_MouseButtonEvent event, GAMESTATE* gamestate);
    void HandleKeyPress(SDL_Keycode key, GAMESTATE* gamestate);
    int PollPromotion(int promotionSquare);

private:
    const int WINDOW_WIDTH = 768;
    const int WINDOW_HEIGHT = 768;
    const int SQ_SIZE = 96;
    const std::unordered_map<int, int> PIECE_NUM_TO_IMAGE_ARRAY_INDEX = {
        {1, 0}, // w_pawns
        {2, 1}, // w_knight
        {3, 2}, // w_bishop
        {4, 3}, // w_rook
        {5, 4}, // w_queen
        {6, 5}, // w_king
        {9, 6}, // b_pawns
        {10, 7}, // b_knight
        {11, 8}, // b_bishop
        {12, 9}, // b_rook
        {13, 10}, // b_queen
        {14, 11}, // b_king
    };

    SDL_Texture* piece_textures[14]{};

    SDL_Renderer* renderer;
    SDL_Window* window;

    std::vector<int> highlightedSqs;
    std::vector<int> selectedSqs;
    std::vector<int> moveIndicatorSqs;

    BoardThemes::ColorTheme theme = BoardThemes::blueTheme;
};
