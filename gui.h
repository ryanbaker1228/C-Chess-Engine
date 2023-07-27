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

    SDL_Color lightSquareColor = { 0xed, 0xd9, 0xb9, 0xff };
    SDL_Color darkSquareColor = { 0xae, 0x89, 0x68, 0xff };
    SDL_Color lightHighlight = { 0xf6, 0xeb, 0x86, 0xff };
    SDL_Color darkHighlight = { 0xd7, 0xc3, 0x5e, 0xff };


    const int sq_colors[4][4] = {
    //    R     G     B     A
        {0xae, 0x89, 0x68, 0xff},       // Dark square color
        {0xd7, 0xc3, 0x5e, 0xff},       // Dark square highlighted
        {0xed, 0xd9, 0xb9, 0xff},       // Light square color
        {0xf6, 0xeb, 0x86, 0xff}        // Light square highlighted
    };
};
