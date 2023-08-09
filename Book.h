//
// Created by Ryan Baker on 8/8/23.
//

#ifndef CHESS_ENGINE_BOOK_H
#define CHESS_ENGINE_BOOK_H

#include "move.h"
#include "gamestate.h"
#include <string>
#include <array>
#include <vector>

class OpeningBook {
private:
    OpeningBook();

    struct Node {
        std::string move;
        int playCount;
        std::vector<Node> children;
    };

    std::vector<Node> data;

public:
    OpeningBook& Get() {
        static OpeningBook instance;
        return instance;
    }

    bool inOpeningBook = true;

    void playBookMove() {

    }
};

#endif //CHESS_ENGINE_BOOK_H
