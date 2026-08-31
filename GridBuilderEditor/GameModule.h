#pragma once

#include "MapPlayerMarker.h"
#include "GameModule.h"

#include <SDL3/SDL.h>

#include <string>
#include <vector>

struct GameButtonRect
{
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;

    Uint8 r = 255;
    Uint8 g = 255;
    Uint8 b = 0;
    Uint8 a = 255;

    std::string dosKey;
};

struct ScreenPixel
{
    int x = 0;
    int y = 0;

    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
};

struct ScreenSignature
{
    std::vector<ScreenPixel>
        pixels;
};

namespace DosBoxX
{
    struct DosBoxFrameHeader;
}

class GameModule
{
public:
    virtual ~GameModule() = default;

    virtual void update() = 0;

    virtual bool playerMarker(
        MapPlayerMarker& marker
    ) const = 0;

    virtual void keyDown(
        SDL_Keycode key
    )
    {}

    virtual bool blockDirectDosBoxKeyboard() const
    {
        return false;
    }

    virtual void keyUp(
        SDL_Keycode key
    )
    {}

    virtual bool takeDosKey(
        std::string& key
    )
    {
        return false;
    }

    virtual bool gameButtonSelection(
        GameButtonRect& rect
    ) const
    {
        rect = {};
        return false;
    }

    virtual void addButtonToActiveWindow(
        const GameButtonRect& rect
    )
    {}

    virtual void updateActiveButton(
        const GameButtonRect& rect
    )
    {}

    virtual void deleteActiveButton()
    {}

    virtual bool takeDosMouseClick(
        float& x,
        float& y
    )
    {
        return false;
    }

    virtual bool takeDosMouseDoubleClick(
        float& x,
        float& y
    )
    {
        return false;
    }

    virtual void setFrame(
        const DosBoxX::DosBoxFrameHeader* frameHeader,
        const uint8_t* framePixels
    )
    {
        m_frameHeader =
            frameHeader;

        m_framePixels =
            framePixels;
    }

    bool matchesScreen(
        const ScreenSignature& signature
    ) const;

    virtual bool takeDosMousePosition(
        float& x,
        float& y
    )
    {
        return false;
    }

protected:
    const DosBoxX::DosBoxFrameHeader*
        m_frameHeader =
        nullptr;

    const uint8_t*
        m_framePixels =
        nullptr;


};