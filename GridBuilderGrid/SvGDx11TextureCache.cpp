#include "SvgDx11TextureCache.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <Windows.h>


SvgDx11TextureCache::SvgDx11TextureCache(
    ID3D11Device* device
)
    :
    m_device(
        device
    )
{}

ID3D11ShaderResourceView*
SvgDx11TextureCache::texture(
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
        m_textures.find(
            key
        );

    if (existing !=
        m_textures.end())
    {
        return existing->second->textureView();
    }

    if (m_device == nullptr ||
        width <= 0 ||
        height <= 0)
    {
        return nullptr;
    }

    SDL_IOStream* stream =
        SDL_IOFromFile(
            filename.c_str(),
            "rb"
        );

    {
        const std::string message =
            "SVG opened: " +
            filename +
            "\n";

        OutputDebugStringA(
            message.c_str()
        );
    }

    if (stream == nullptr)
    {
        const std::string message =
            "SVG open failed: " +
            filename +
            " | " +
            SDL_GetError() +
            "\n";

        OutputDebugStringA(
            message.c_str()
        );

        return nullptr;
    }

    if (stream == nullptr)
    {
        return nullptr;
    }

    SDL_Surface* surface =
        IMG_LoadSizedSVG_IO(
            stream,
            width,
            height
        );

    SDL_CloseIO(
        stream
    );

    if (surface == nullptr)
    {
        return nullptr;
    }

    SDL_Surface* rgbaSurface =
        SDL_ConvertSurface(
            surface,
            SDL_PIXELFORMAT_RGBA32
        );

    SDL_DestroySurface(
        surface
    );

    if (rgbaSurface == nullptr)
    {
        return nullptr;
    }

    auto texture =
        std::make_unique<Dx11Texture>(
            m_device
        );

    const bool created =
        texture->create(
            static_cast<const uint8_t*>(
                rgbaSurface->pixels
                ),
            static_cast<uint32_t>(
                rgbaSurface->w
                ),
            static_cast<uint32_t>(
                rgbaSurface->h
                ),
            static_cast<uint32_t>(
                rgbaSurface->pitch
                )
        );

    SDL_DestroySurface(
        rgbaSurface
    );

    if (!created)
    {
        return nullptr;
    }

    ID3D11ShaderResourceView* textureView =
        texture->textureView();

    m_textures.emplace(
        key,
        std::move(texture)
    );

    return textureView;
}

void SvgDx11TextureCache::clear()
{
    m_textures.clear();
}

std::string
SvgDx11TextureCache::createKey(
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