#include "SvgTextureCache.h"

#include <SDL3_image/SDL_image.h>

#include <string>

SvgTextureCache::SvgTextureCache(
    SDL_Renderer* renderer
)
    : m_renderer(renderer)
{}

SvgTextureCache::~SvgTextureCache()
{
    clear();
}

SDL_Texture* SvgTextureCache::texture(
    const std::string& filename,
    int width,
    int height
)
{
    const std::string key =
        createKey(
            filename,
            width,
            height
        );

    const auto existing =
        m_textures.find(key);

    if (existing != m_textures.end())
    {
        return existing->second;
    }

    SDL_IOStream* stream =
        SDL_IOFromFile(
            filename.c_str(),
            "rb"
        );

    if (stream == nullptr)
    {
        SDL_Log(
            "Failed to open SVG '%s': %s",
            filename.c_str(),
            SDL_GetError()
        );

        return nullptr;
    }

    SDL_Surface* surface =
        IMG_LoadSizedSVG_IO(
            stream,
            width,
            height
        );

    SDL_CloseIO(stream);

    if (surface == nullptr)
    {
        SDL_Log(
            "Failed to load SVG '%s': %s",
            filename.c_str(),
            SDL_GetError()
        );

        return nullptr;
    }

    SDL_Texture* newTexture =
        SDL_CreateTextureFromSurface(
            m_renderer,
            surface
        );

    SDL_DestroySurface(surface);

    if (newTexture == nullptr)
    {
        SDL_Log(
            "Failed to create texture '%s': %s",
            filename.c_str(),
            SDL_GetError()
        );

        return nullptr;
    }

    m_textures.emplace(
        key,
        newTexture
    );

    return newTexture;
}

void SvgTextureCache::clear()
{
    for (const auto& entry : m_textures)
    {
        SDL_DestroyTexture(entry.second);
    }

    m_textures.clear();
}

std::string SvgTextureCache::createKey(
    const std::string& filename,
    int width,
    int height
) const
{
    return filename +
        "#" +
        std::to_string(width) +
        "x" +
        std::to_string(height);
}