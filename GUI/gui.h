//
// Created by Ryan Baker on 6/12/23.
//

#pragma once
#include "../gamestate.h"
#include "../Zobrist.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <string>
#include <vector>
#include <array>
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
private:
    GUI();
    ~GUI();

    void LoadTextures();
    void DrawBoard();
    void DrawPieces();
    void DrawIndicators();

    int PollPromotion(int promotionSquare);

    const int WINDOW_WIDTH = 768;
    const int WINDOW_HEIGHT = 768;
    const int SQ_SIZE = 96;

    std::array<SDL_Texture*, 15> piece_textures;

    SDL_Renderer* renderer;
    SDL_Window* window;

    std::stack<Move> backupMoveLog;

    std::vector<int> highlightedSqs;
    std::vector<int> selectedSqs;
    std::vector<int> moveIndicatorSqs;

    BoardThemes::ColorTheme theme = BoardThemes::blueTheme;
public:
    static GUI& Get() {
        static GUI instance;
        return instance;
    }

    void HandleButtonClick();
    void HandleKeyPress(SDL_Keycode key);
    void DrawGame();
    void UpdateHighlights();
};
