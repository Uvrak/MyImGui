#pragma once

#include <SDL3/SDL.h>
#include <cstdint>

class DosBoxFrameTextureSDL
{
public:
    explicit DosBoxFrameTextureSDL(
        SDL_Renderer* renderer
    );

    ~DosBoxFrameTextureSDL();

    bool update(
        const uint8_t* pixels,
        uint32_t width,
        uint32_t height,
        uint32_t pitch
    );

    SDL_Texture* texture() const;

private:
    SDL_Renderer* m_renderer =
        nullptr;

    SDL_Texture* m_texture =
        nullptr;

    uint32_t m_width = 0;
    uint32_t m_height = 0;
};
