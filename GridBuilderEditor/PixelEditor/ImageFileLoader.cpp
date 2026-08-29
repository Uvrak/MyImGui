#include "ImageFileLoader.h"

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

#include <cstdint>

#include <utility>

bool ImageFileLoader::load(
    const std::string& filename,
    PixelImage& image
)
{
    SDL_Surface* loadedSurface =
        IMG_Load(filename.c_str());

    if (loadedSurface == nullptr)
    {
        SDL_Log(
            "Failed to load image: %s",
            SDL_GetError()
        );

        return false;
    }

    SDL_Surface* rgbaSurface =
        SDL_ConvertSurface(
            loadedSurface,
            SDL_PIXELFORMAT_RGBA32
        );

    SDL_DestroySurface(loadedSurface);

    if (rgbaSurface == nullptr)
    {
        SDL_Log(
            "Failed to convert image: %s",
            SDL_GetError()
        );

        return false;
    }

    const bool mustLock =
        SDL_MUSTLOCK(rgbaSurface);

    if (mustLock &&
        !SDL_LockSurface(rgbaSurface))
    {
        SDL_Log(
            "Failed to lock image: %s",
            SDL_GetError()
        );

        SDL_DestroySurface(rgbaSurface);
        return false;
    }

    PixelImage loadedImage(
        rgbaSurface->w,
        rgbaSurface->h
    );

    const auto* pixels =
        static_cast<const std::uint8_t*>(
            rgbaSurface->pixels
            );

    for (int y = 0;
        y < rgbaSurface->h;
        ++y)
    {
        const std::uint8_t* row =
            pixels +
            y * rgbaSurface->pitch;

        for (int x = 0;
            x < rgbaSurface->w;
            ++x)
        {
            const std::uint8_t* source =
                row + x * 4;

            loadedImage.pixel(x, y) =
            {
                source[0],
                source[1],
                source[2],
                source[3]
            };
        }
    }

    if (mustLock)
    {
        SDL_UnlockSurface(rgbaSurface);
    }

    SDL_DestroySurface(rgbaSurface);

    image =
        std::move(loadedImage);

    return true;
}