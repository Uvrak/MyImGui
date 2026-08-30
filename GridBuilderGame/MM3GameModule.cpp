#include "MM3GameModule.h"

#include "StateReader.h"
#include "Position.h"

#include <Windows.h>

MM3GameModule::MM3GameModule(
    MightAndMagic3::StateReader& stateReader
)
    :
    m_stateReader(
        stateReader
    )
{
    m_keyBindings.setDefaultKey(
        MightAndMagic3::KeyAction::ToggleButtonMode,
        static_cast<int>(SDLK_END)
    );

    m_keyBindings.setDefaultKey(
        MightAndMagic3::KeyAction::Forward,
        static_cast<int>(SDLK_UP)
    );

    m_keyBindings.setDefaultKey(
        MightAndMagic3::KeyAction::Backward,
        static_cast<int>(SDLK_DOWN)
    );

    m_keyBindings.setDefaultKey(
        MightAndMagic3::KeyAction::TurnLeft,
        static_cast<int>(SDLK_LEFT)
    );

    m_keyBindings.setDefaultKey(
        MightAndMagic3::KeyAction::TurnRight,
        static_cast<int>(SDLK_RIGHT)
    );

    m_keyBindings.setDefaultKey(
        MightAndMagic3::KeyAction::ButtonUp,
        static_cast<int>(SDLK_UP)
    );

    m_keyBindings.setDefaultKey(
        MightAndMagic3::KeyAction::ButtonDown,
        static_cast<int>(SDLK_DOWN)
    );

    m_keyBindings.setDefaultKey(
        MightAndMagic3::KeyAction::ButtonLeft,
        static_cast<int>(SDLK_LEFT)
    );

    m_keyBindings.setDefaultKey(
        MightAndMagic3::KeyAction::ButtonRight,
        static_cast<int>(SDLK_RIGHT)
    );

    m_keyBindings.setDefaultKey(
        MightAndMagic3::KeyAction::Cancel,
        static_cast<int>(SDLK_ESCAPE)
    );

    m_keyBindings.setDefaultKey(
        MightAndMagic3::KeyAction::ActivateButton,
        static_cast<int>(SDLK_RETURN)
    );
}

bool MM3GameModule::
blockDirectDosBoxKeyboard() const
{
    return m_buttonMode;
}

void MM3GameModule::update()
{
    m_stateReader.update(
        m_state
    );
}

bool MM3GameModule::playerMarker(
    MapPlayerMarker& marker
) const
{
    const MightAndMagic3::Position&
        position =
        m_state.position();

    if (!position.valid)
    {
        marker.visible = false;
        return false;
    }

    marker.x =
        position.x;

    marker.y =
        -position.y;

    switch (position.direction)
    {
    case MightAndMagic3::Direction::North:
        marker.direction =
            MapFacingDirection::North;
        break;

    case MightAndMagic3::Direction::East:
        marker.direction =
            MapFacingDirection::East;
        break;

    case MightAndMagic3::Direction::South:
        marker.direction =
            MapFacingDirection::South;
        break;

    case MightAndMagic3::Direction::West:
        marker.direction =
            MapFacingDirection::West;
        break;

    case MightAndMagic3::Direction::Unknown:
    default:
        marker.visible = false;
        return false;
    }

    marker.visible = true;

    return true;
}

bool MM3GameModule::gameButtonSelection(
    int& selectedButton
) const
{
    if (!m_buttonMode)
    {
        selectedButton = -1;
        return false;
    }

    selectedButton =
        m_selectedButton;

    return
        m_selectedButton >= 0;
}

void MM3GameModule::keyUp(
    SDL_Keycode key
)
{}

bool MM3GameModule::takeDosKey(
    std::string& key
)
{
    if (m_pendingDosKey.empty())
    {
        return false;
    }

    key =
        m_pendingDosKey;

    m_pendingDosKey.clear();

    return true;
}

void MM3GameModule::keyDown(
    SDL_Keycode key
)
{
    auto matches =
        [this, key](
            MightAndMagic3::KeyAction action
            )
        {
            const int configuredKey =
                m_keyBindings.key(
                    action
                );

            return
                configuredKey != 0 &&
                configuredKey ==
                static_cast<int>(
                    key
                    );
        };

    if (matches(
        MightAndMagic3::KeyAction::
        ToggleButtonMode
    ))
    {
        m_buttonMode =
            !m_buttonMode;

        m_selectedButton =
            m_buttonMode
            ? 0
            : -1;

        OutputDebugStringA(
            m_buttonMode
            ? "MM3 Button Mode: ON\n"
            : "MM3 Button Mode: OFF\n"
        );

        return;
    }

    if (matches(
        MightAndMagic3::KeyAction::
        Cancel
    ))
    {
        m_pendingDosKey =
            "ESC";

        return;
    }

    if (m_buttonMode)
    {
        if (matches(
            MightAndMagic3::KeyAction::
            ButtonUp
        ))
        {
            m_selectedButton =
                0;

            return;
        }

        if (matches(
            MightAndMagic3::KeyAction::
            ButtonDown
        ))
        {
            m_selectedButton =
                1;

            return;
        }

        return;
    }

    if (matches(
        MightAndMagic3::KeyAction::
        Forward
    ))
    {
        m_pendingDosKey =
            "UP";

        return;
    }

    if (matches(
        MightAndMagic3::KeyAction::
        Backward
    ))
    {
        m_pendingDosKey =
            "DOWN";

        return;
    }

    if (matches(
        MightAndMagic3::KeyAction::
        TurnLeft
    ))
    {
        m_pendingDosKey =
            "LEFT";

        return;
    }

    if (matches(
        MightAndMagic3::KeyAction::
        TurnRight
    ))
    {
        m_pendingDosKey =
            "RIGHT";

        return;
    }
}