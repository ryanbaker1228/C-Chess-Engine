//
// Created by Ryan Baker on 8/4/23.
//

#ifndef CHESS_ENGINE_BOT_H
#define CHESS_ENGINE_BOT_H


class Bot {
private:
    Bot();

public:
    static Bot& Get() {
        static Bot instance;
        return instance;
    }

    bool bot_to_play = true;
    bool playing_white = false;
    bool playing_black = true;

    void PlayMove();
};


#endif //CHESS_ENGINE_BOT_H
