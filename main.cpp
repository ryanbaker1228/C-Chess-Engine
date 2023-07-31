#include "gui.h"
#include "gamestate.h"
#include "movegen.h"
#include "evaluation.h"
#include <chrono>
#include <iostream>


int main() {
    Gamestate& gamestate = Gamestate::Get();
    MoveGenerator& moveGenerator = MoveGenerator::Get();
    MovementTables::LoadTables();
    GUI gui;
    SDL_Event event;
    moveGenerator.PerftTest();

    bool running = true;
    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    gui.HandleButtonClick(event.button);
                    break;
                case SDL_KEYDOWN:
                    SDL_Keycode key = event.key.keysym.sym;
                    gui.HandleKeyPress(key);
                    break;
            }
        }
        gui.DrawGame();
    }
}