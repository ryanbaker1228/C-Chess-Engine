#include "GUI/gui.h"
#include "gamestate.h"
#include "Search.h"
#include "evaluation.h"
#include "movegen.h"
#include "Test.h"
#include <iostream>


int main() {
    Gamestate& gamestate = Gamestate::Get();
    MovementTables::LoadTables();
    GUI& gui = GUI::Get();
    SDL_Event event;
    //SearchTest::TestSearch();
    gamestate.Seed("2b1kb2/8/8/8/3K4/8/8/8");
    gamestate.zobristKey = Zobrist::Get().GenerateKey();

    bool running = true;
    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = false;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    gui.HandleButtonClick();
                    break;
                case SDL_KEYDOWN:
                    SDL_Keycode key = event.key.keysym.sym;
                    gui.HandleKeyPress(key);
                    break;
            }
        }
        gui.DrawGame();
        MoveGenerator::Get().GenerateLegalMoves();
        if (!Gamestate::Get().whiteToMove) {
            MovePicker::Get().IterativeDeepeningSearch();
            gamestate.MakeMove(MovePicker::Get().bestMove);
            std::cout << Zobrist::Get().GenerateKey() << std::endl;
            std::cout << TranspositionTable::Get().positions.size() << std::endl;
            GUI::Get().UpdateHighlights();
        }
    }
}