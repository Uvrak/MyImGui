#pragma once

#include <SDL3/SDL.h>

#include <string>
#include <unordered_map>

class SvgTextureCache
{
public:
    explicit SvgTextureCache(
        SDL_Renderer* renderer
    );

    ~SvgTextureCache();

    SDL_Texture* texture(
        const std::string& filename,
        int width,
        int height
    );

    void clear();

private:
    std::string createKey(
        const std::string& filename,
        int width,
        int height
    ) const;

private:
    SDL_Renderer* m_renderer = nullptr;

    std::unordered_map<
        std::string,
        SDL_Texture*
    > m_textures;
};
