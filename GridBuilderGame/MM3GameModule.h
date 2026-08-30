#pragma once

#include "GameModule.h"
#include "GameState.h"
#include "MM3KeyBindings.h"

#include <string>

namespace DosBoxX
{
    class Controller;
    class NamedPipeClient;
}

namespace MightAndMagic3
{
    class StateReader;
}

class MM3GameModule final :
    public GameModule
{
public:
    explicit MM3GameModule(
        MightAndMagic3::StateReader& stateReader
    );

    bool blockDirectDosBoxKeyboard() const override;

    void update() override;

    void keyDown(
        SDL_Keycode key
    ) override;

    void keyUp(
        SDL_Keycode key
    ) override;

    bool takeDosKey(
        std::string& key
    ) override;

    bool playerMarker(
        MapPlayerMarker& marker
    ) const override;

    bool buttonMode() const
    {
        return m_buttonMode;
    }

    int selectedButton() const
    {
        return m_selectedButton;
    }

    bool gameButtonSelection(
        int& selectedButton
    ) const override;

private:
    bool m_buttonMode = false;

    MightAndMagic3::StateReader&
        m_stateReader;

    MightAndMagic3::GameState
        m_state;

    MightAndMagic3::KeyBindings
        m_keyBindings;

    std::string m_pendingDosKey;

    int m_selectedButton = -1;

};