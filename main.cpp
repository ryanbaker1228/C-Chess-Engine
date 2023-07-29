#include "gui.h"
#include "gamestate.h"
#include "movegen.h"
#include "evaluation.h"
#include <chrono>
#include <iostream>


int main() {
    MovementTables::LoadTables();
    //MoveGenerator::TestMoveGenerator();
    GUI gui;
    SDL_Event event;
    GAMESTATE gamestate = GAMESTATE("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq -");;

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