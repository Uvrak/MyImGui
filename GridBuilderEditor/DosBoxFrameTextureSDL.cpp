#include "DosBoxFrameTextureSDL.h"

DosBoxFrameTextureSDL::DosBoxFrameTextureSDL(
    SDL_Renderer* renderer
)
    : m_renderer(renderer)
{}

DosBoxFrameTextureSDL::~DosBoxFrameTextureSDL()
{
    if (m_texture != nullptr)
    {
        SDL_DestroyTexture(
            m_texture
        );

        m_texture = nullptr;
    }
}

bool DosBoxFrameTextureSDL::update(
    const uint8_t* pixels,
    uint32_t width,
    uint32_t height,
    uint32_t pitch
)
{
    if (m_renderer == nullptr ||
        pixels == nullptr ||
        width == 0 ||
        height == 0 ||
        pitch == 0)
    {
        return false;
    }

    if (m_texture == nullptr ||
        m_width != width ||
        m_height != height)
    {
        if (m_texture != nullptr)
        {
            SDL_DestroyTexture(
                m_texture
            );

            m_texture = nullptr;
        }

        m_texture =
            SDL_CreateTexture(
                m_renderer,
                SDL_PIXELFORMAT_XRGB8888,
                SDL_TEXTUREACCESS_STREAMING,
                static_cast<int>(width),
                static_cast<int>(height)
            );

        if (m_texture == nullptr)
        {
            return false;
        }

        m_width = width;
        m_height = height;
    }

    if (!SDL_UpdateTexture(
        m_texture,
        nullptr,
        pixels,
        static_cast<int>(pitch)
    ))
    {
        return false;
    }

    return true;
}

SDL_Texture*
DosBoxFrameTextureSDL::texture() const
{
    return m_texture;
}