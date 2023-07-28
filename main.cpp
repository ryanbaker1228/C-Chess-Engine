#include "gui.h"
#include "gamestate.h"
#include "movegen.h"
#include "evaluation.h"
#include <chrono>
#include <iostream>


int main() {
    MovementTables::LoadTables();
    MoveGenerator::TestMoveGenerator();
    GUI gui;
    SDL_Event event;
    GAMESTATE gamestate = GAMESTATE("r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1");
    //auto start = std::chrono::high_resolution_clock::now();
    //std::cout << MoveSearch(&gamestate, 6, BLACK_WIN, WHITE_WIN) << std::endl;
    //auto stop = std::chrono::high_resolution_clock::now();
    //auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(stop - start);
    //std::cout << float(duration.count()) / pow(10, 9) << std::endl;

    bool running = true;
    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    gui.HandleButtonClick(event.button, &gamestate);
                    break;
                case SDL_KEYDOWN:
                    SDL_Keycode key = event.key.keysym.sym;
                    gui.HandleKeyPress(key, &gamestate);
                    break;
            }
        }
        gui.DrawGame(&gamestate);
    }
}