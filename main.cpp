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
    std::cout << atan2(0, 1) * 180 / 3.14159 << std::endl;
    std::cout << atan2(1, 0) * 180 / 3.14159265358979 << std::endl;
    std::cout << atan2(0.5, 0.5) * 180 / 3.14159 << std::endl;

    Gamestate& gamestate = Gamestate::Get();
    MovementTables::LoadTables();
    GUI& gui = GUI::Get();
    SDL_Event event;
    //SearchTest::TestSearch();
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
                case SDL_MOUSEBUTTONUP:
                    gui.HandleButtonClick(event.button);
                    break;
                case SDL_KEYDOWN:
                    SDL_Keycode key = event.key.keysym.sym;
                    gui.HandleKeyPress(key);
                    break;
            }
        }
        gui.DrawGame();
        MoveGenerator::Get().GenerateLegalMoves();
        if ((Gamestate::Get().whiteToMove) or true or gamestate.result == Pending) {
            std::vector<Move> legalMoves = MoveGenerator::Get().GenerateLegalMoves();
            MoveOrderer::Get().OrderMoves(&legalMoves);
            int i = 0;
            for (auto move : legalMoves) {
                ++i;
                if (i > 5) break;
                gui.DrawArrow(move.startSquare, move.endSquare);
            }
            //MovePicker::Get().InitSearch();
            //std::cout << PGNNotation(MovePicker::Get().bestMove) << ", ";
           // std::cout << float(TranspositionTable::Get().positions.size() * sizeof(TranspositionTable::Get().positions[0])) / 1000000  << '\n';
            //gamestate.MakeMove(MovePicker::Get().bestMove);
            //GUI::Get().UpdateHighlights();
        }
    }
}