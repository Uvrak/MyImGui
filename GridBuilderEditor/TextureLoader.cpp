#include "TextureLoader.h"

#include <SDL3_image/SDL_image.h>

SDL_Texture* TextureLoader::load(
    SDL_Renderer* renderer,
    const char* fileName)
{
    return IMG_LoadTexture(
        renderer,
        fileName
    );
}