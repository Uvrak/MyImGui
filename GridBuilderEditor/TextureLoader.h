#pragma once

#include <SDL3/SDL.h>

class TextureLoader
{
public:
    static SDL_Texture* load(
        SDL_Renderer* renderer,
        const char* fileName
    );
};