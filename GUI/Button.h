//
// Created by Ryan Baker on 8/7/23.
//

#ifndef CHESS_ENGINE_BUTTON_H
#define CHESS_ENGINE_BUTTON_H

#include <SDL2/SDL.h>

class Button {
public:
    SDL_Texture* texture;
    SDL_Rect srcRect, destRect;
    bool hoveringOver = false;

    Button();
    virtual ~Button();

    void update();
};


#endif //CHESS_ENGINE_BUTTON_H
