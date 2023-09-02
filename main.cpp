#include "GUI/gui.h"
#include "gamestate.h"
#include "Search.h"
#include "Test.h"
#include "movegen.h"
#include "bitUtils.h"
#include <iostream>
#include <chrono>
#include <random>


int main() {
    Gamestate& gamestate = Gamestate::Get();
    MovementTables::LoadTables();
    GUI& gui = GUI::Get();
    SDL_Event event;
    //MoveGenTest::TestPerft();
    gamestate.Seed();
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
        if (!Gamestate::Get().whiteToMove or true) {
            MovePicker::Get().InitSearch();
            //std::cout << PGNNotation(MovePicker::Get().bestMove) << ", ";
           // std::cout << float(TranspositionTable::Get().positions.size() * sizeof(TranspositionTable::Get().positions[0])) / 1000000  << '\n';
            gamestate.MakeMove(MovePicker::Get().bestMove);
            GUI::Get().UpdateHighlights();
        }
    }
}