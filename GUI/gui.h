//
// Created by Ryan Baker on 6/12/23.
//

#pragma once
#include "../gamestate.h"
#include "../Zobrist.h"
#include <SDL2/SDL.h>
#include <SDL_image.h>
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
    void DrawArrows();

    int PollPromotion(int promotionSquare);

    const int windowWidth = 768;
    const int windowHeight = 768;
    const int sqSize = 96;

    std::array<SDL_Texture*, 15> piece_textures;
    std::array<SDL_Texture*, 33> arrow_textures;

    SDL_Renderer* renderer;
    SDL_Window* window;

    std::stack<Move> backupMoveLog;

    std::vector<int> highlightedSqs;
    std::vector<int> selectedSqs;
    int arrow_start_square;
    std::vector<int> moveIndicatorSqs;
    std::vector<std::pair<int, int>> drawnArrows;
    BoardThemes::ColorTheme theme = BoardThemes::blueTheme;

public:
    static GUI& Get() {
        static GUI instance;
        return instance;
    }

    void HandleButtonClick(SDL_MouseButtonEvent event);
    void HandleKeyPress(SDL_Keycode key);
    void DrawGame();
    void UpdateHighlights();
    void DrawArrow(int start_sq, int end_sq) {
        drawnArrows.push_back({start_sq, end_sq});
    }

    bool flipBoard = false;
};

inline std::unordered_map<std::string, std::pair<int, SDL_RendererFlip>> arrow_map{
        {"{1, 0}", {0, SDL_FLIP_NONE}},
        {"{2, 0}", {1, SDL_FLIP_NONE}},
        {"{3, 0}", {2, SDL_FLIP_NONE}},
        {"{4, 0}", {3, SDL_FLIP_NONE}},
        {"{5, 0}", {4, SDL_FLIP_NONE}},
        {"{6, 0}", {5, SDL_FLIP_NONE}},
        {"{7, 0}", {6, SDL_FLIP_NONE}},
        {"{-1, 0}", {0, SDL_FLIP_VERTICAL}},
        {"{-2, 0}", {1, SDL_FLIP_VERTICAL}},
        {"{-3, 0}", {2, SDL_FLIP_VERTICAL}},
        {"{-4, 0}", {3, SDL_FLIP_VERTICAL}},
        {"{-5, 0}", {4, SDL_FLIP_VERTICAL}},
        {"{-6, 0}", {5, SDL_FLIP_VERTICAL}},
        {"{-7, 0}", {6, SDL_FLIP_VERTICAL}},

        {"{0, 1}", {7, SDL_FLIP_NONE}},
        {"{0, 2}", {8, SDL_FLIP_NONE}},
        {"{0, 3}", {9, SDL_FLIP_NONE}},
        {"{0, 4}", {10, SDL_FLIP_NONE}},
        {"{0, 5}", {11, SDL_FLIP_NONE}},
        {"{0, 6}", {12, SDL_FLIP_NONE}},
        {"{0, 7}", {13, SDL_FLIP_NONE}},
        {"{0, -1}", {7, SDL_FLIP_HORIZONTAL}},
        {"{0, -2}", {8, SDL_FLIP_HORIZONTAL}},
        {"{0, -3}", {9, SDL_FLIP_HORIZONTAL}},
        {"{0, -4}", {10, SDL_FLIP_HORIZONTAL}},
        {"{0, -5}", {11, SDL_FLIP_HORIZONTAL}},
        {"{0, -6}", {12, SDL_FLIP_HORIZONTAL}},
        {"{0, -7}", {13, SDL_FLIP_HORIZONTAL}},

        {"{1, 1}", {14, SDL_FLIP_NONE}},
        {"{2, 2}", {15, SDL_FLIP_NONE}},
        {"{3, 3}", {16, SDL_FLIP_NONE}},
        {"{4, 4}", {17, SDL_FLIP_NONE}},
        {"{5, 5}", {18, SDL_FLIP_NONE}},
        {"{6, 6}", {19, SDL_FLIP_NONE}},
        {"{7, 7}", {20, SDL_FLIP_NONE}},
        {"{-1, 1}", {14, SDL_FLIP_VERTICAL}},
        {"{-2, 2}", {15, SDL_FLIP_VERTICAL}},
        {"{-3, 3}", {16, SDL_FLIP_VERTICAL}},
        {"{-4, 4}", {17, SDL_FLIP_VERTICAL}},
        {"{-5, 5}", {18, SDL_FLIP_VERTICAL}},
        {"{-6, 6}", {19, SDL_FLIP_VERTICAL}},
        {"{-7, 7}", {20, SDL_FLIP_VERTICAL}},

        {"{1, -1}", {21, SDL_FLIP_NONE}},
        {"{2, -2}", {22, SDL_FLIP_NONE}},
        {"{3, -3}", {23, SDL_FLIP_NONE}},
        {"{4, -4}", {24, SDL_FLIP_NONE}},
        {"{5, -5}", {25, SDL_FLIP_NONE}},
        {"{6, -6}", {26, SDL_FLIP_NONE}},
        {"{7, -7}", {27, SDL_FLIP_NONE}},
        {"{-1, -1}", {21, SDL_FLIP_VERTICAL}},
        {"{-2, -2}", {22, SDL_FLIP_VERTICAL}},
        {"{-3, -3}", {23, SDL_FLIP_VERTICAL}},
        {"{-4, -4}", {24, SDL_FLIP_VERTICAL}},
        {"{-5, -5}", {25, SDL_FLIP_VERTICAL}},
        {"{-6, -6}", {26, SDL_FLIP_VERTICAL}},
        {"{-7, -7}", {27, SDL_FLIP_VERTICAL}},

        {"{1, 2}", {28, SDL_FLIP_NONE}},
        {"{-1, 2}", {28, SDL_FLIP_VERTICAL}},
        {"{2, 1}", {29, SDL_FLIP_NONE}},
        {"{-2, 1}", {29, SDL_FLIP_VERTICAL}},
        {"{2, -1}", {30, SDL_FLIP_NONE}},
        {"{-2, -1}", {30, SDL_FLIP_VERTICAL}},
        {"{1, -2}", {31, SDL_FLIP_NONE}},
        {"{-1, -2}", {31, SDL_FLIP_VERTICAL}},

        {"{0, 0}", {32, SDL_FLIP_NONE}},
};
