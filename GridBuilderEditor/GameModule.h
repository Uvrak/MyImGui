#pragma once

#include "MapPlayerMarker.h"

#include <SDL3/SDL.h>

#include <string>

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
        int& selectedButton
    ) const
    {
        selectedButton = -1;
        return false;
    }
};