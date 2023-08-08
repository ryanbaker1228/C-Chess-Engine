#include "gui.h"
#include "gamestate.h"
#include "Search.h"
#include "movegen.h"
#include "Test.h"
#include <chrono>
#include <iostream>


int main() {
    Gamestate& gamestate = Gamestate::Get();
    MovementTables::LoadTables();
    GUI gui;
    SDL_Event event;
    //MoveGenTest::PerftTest(); // 562 seconds full test
    gamestate.Seed();

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
        if (!gamestate.whiteToMove) {
            MovePicker::Get().MiniMaxSearch(4, constantEvals::negativeInfinity, constantEvals::positiveInfinity);
            gamestate.MakeMove(MovePicker::Get().bestMove);
        }
    }
}