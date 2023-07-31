#include "gui.h"
#include "gamestate.h"
#include "movegen.h"
#include "evaluation.h"
#include <chrono>
#include <iostream>


int main() {
    MoveGenerator* moveGenerator = MoveGenerator::Get();
    MovementTables::LoadTables();
    GUI gui;
    SDL_Event event;
    moveGenerator->PerftTest();
    GAMESTATE gamestate = GAMESTATE("8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - -");
    moveGenerator->Seed(gamestate);

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
    delete moveGenerator;
}