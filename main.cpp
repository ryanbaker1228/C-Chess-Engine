#include "GUI/gui.h"
#include "gamestate.h"
#include "Search.h"
#include "Test.h"
#include "movegen.h"
#include "Bot.h"
#include "bitUtils.h"
#include "evaluation.h"
#include "Transposition.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <random>


int main() {
    Gamestate& gamestate = Gamestate::Get();
    MovementTables::LoadTables();
    GUI& gui = GUI::Get();
    TranspositionTable& tt = TranspositionTable::Get();
    SDL_Event event;
    //SearchTest::TestSearch();
    gamestate.Seed(/*"r1bqk2r/ppppbppp/2nn4/1B2N3/8/8/PPPP1PPP/RNBQR1K1 w kq - 1 7"*/);
    gamestate.zobristKey = Zobrist::Get().GenerateKey();
    std::thread ttCleaner (CleanUp, &tt);

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
        Bot::Get().PlayMove();
        MoveGenerator::Get().GenerateLegalMoves();
    }
    ttCleaner.join();
    return 0;
}