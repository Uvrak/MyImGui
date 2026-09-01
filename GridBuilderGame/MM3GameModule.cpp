#include "MM3GameModule.h"

#include "StateReader.h"
#include "ScreenSignatures.h"

#include "Position.h"

#include <Windows.h>
#include <cstdio>
#include <fstream>
#include <filesystem>

MM3GameModule::MM3GameModule(
    MightAndMagic3::StateReader& stateReader
)
    :
    m_stateReader(
        stateReader
    )
{
    OutputDebugStringA(
        std::filesystem::current_path()
        .string()
        .c_str()
    );

    OutputDebugStringA(
        "\n"
    );

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

    MM3Window unknownWindow;

    unknownWindow.name =
        "Unknown";

    m_windows.push_back(
        unknownWindow
    );


    MM3Window loadingScreen;

    loadingScreen.name =
        "Loading Screen";

    m_windows.push_back(
        loadingScreen
    );


    MM3Window loadGameWindow;

    loadGameWindow.name =
        "Load Game";

    m_windows.push_back(
        loadGameWindow
    );


    MM3Window mainGameWindow;

    mainGameWindow.name =
        "Main Game";

    m_windows.push_back(
        mainGameWindow
    );


    m_activeWindow =
        0;

    m_selectedButton =
        -1;

    loadButtons();

    for (size_t i = 0;
        i < m_windows.size();
        ++i)
    {
        char debugText[256] = {};

        std::snprintf(
            debugText,
            sizeof(debugText),
            "MM3 WINDOW %zu [%s] buttons=%zu\n",
            i,
            m_windows[i].name.c_str(),
            m_windows[i].buttons.size()
        );

        OutputDebugStringA(
            debugText
        );
    }

    if (!m_windows[
        m_activeWindow
    ].buttons.empty())
    {
        m_selectedButton =
            1;
    }

    char text[128] = {};

    std::snprintf(
        text,
        sizeof(text),
        "Loaded MM3 buttons: %zu\n",
        m_windows[
            m_activeWindow
        ].buttons.size()
                );

    OutputDebugStringA(
        text
    );
}

void MM3GameModule::addButtonToActiveWindow(
    const GameButtonRect& rect
)
{
    if (m_activeWindow < 0 ||
        m_activeWindow >=
        static_cast<int>(
            m_windows.size()
            ))
    {
        return;
    }

    m_windows[
        m_activeWindow
    ].buttons.push_back(
        rect
    );

        m_selectedButton =
            static_cast<int>(
                m_windows[
                    m_activeWindow
                ].buttons.size()
                        ) - 1;
    
    saveButtons();
}

void MM3GameModule::saveButtons() const
{
    std::ofstream file(
        "../settings/mm3_buttons.cfg",
        std::ios::trunc
    );

    if (!file.is_open())
    {
        return;
    }

    for (const MM3Window& window :
        m_windows)
    {
        file
            << "window "
            << window.name
            << '\n';

        for (const GameButtonRect& button :
            window.buttons)
        {
            file
                << "button "
                << button.x << ' '
                << button.y << ' '
                << button.width << ' '
                << button.height << ' '
                << static_cast<int>(button.r) << ' '
                << static_cast<int>(button.g) << ' '
                << static_cast<int>(button.b) << ' '
                << static_cast<int>(button.a) << ' '
                << '\n';
        }
    }
}

void MM3GameModule::loadButtons()
{
    std::ifstream file(
        "../settings/mm3_buttons.cfg"
    );

    if (!file.is_open())
    {
        return;
    }

    std::string type;

    int currentWindow =
        -1;

    while (file >> type)
    {
        if (type == "window")
        {
            std::string name;

            std::getline(
                file >> std::ws,
                name
            );

            for (size_t i = 0;
                i < m_windows.size();
                ++i)
            {
                if (m_windows[i].name ==
                    name)
                {
                    currentWindow =
                        static_cast<int>(
                            i
                            );

                    break;
                }
            }
        }
        else if (
            type == "button" &&
            currentWindow >= 0)
        {
            GameButtonRect button;

            int r = 255;
            int g = 255;
            int b = 0;
            int a = 255;

            file
                >> button.x
                >> button.y
                >> button.width
                >> button.height
                >> r
                >> g
                >> b
                >> a;
                
            button.r =
                static_cast<uint8_t>(r);

            button.g =
                static_cast<uint8_t>(g);

            button.b =
                static_cast<uint8_t>(b);

            button.a =
                static_cast<uint8_t>(a);

            m_windows[
                currentWindow
            ].buttons.push_back(
                button
            );
        }
    }
}

bool MM3GameModule::takeDosMouseDoubleClick(
    float& x,
    float& y
)
{
    return false;
}

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

bool MM3GameModule::takeDosMouseClick(
    float& x,
    float& y
)
{
    if (!m_pendingMouseClick)
    {
        return false;
    }

    m_pendingMouseClick =
        false;

    if (m_activeWindow < 0 ||
        m_activeWindow >=
        static_cast<int>(
            m_windows.size()
            ))
    {
        return false;
    }

    const MM3Window& window =
        m_windows[
            m_activeWindow
        ];

    if (m_selectedButton < 0 ||
        m_selectedButton >=
        static_cast<int>(
            window.buttons.size()
            ))
    {
        return false;
    }

    const GameButtonRect& button =
        window.buttons[
            m_selectedButton
        ];

    x =
        button.x +
        button.width * 0.5f;

    y =
        button.y +
        button.height * 0.5f +
        10.0f;

    return true;
}

bool MM3GameModule::takeDosMousePosition(
    float& x,
    float& y
)
{
    if (!m_pendingMousePosition)
    {
        return false;
    }

    m_pendingMousePosition =
        false;

    if (m_activeWindow < 0 ||
        m_activeWindow >=
        static_cast<int>(
            m_windows.size()
            ))
    {
        return false;
    }

    const MM3Window& window =
        m_windows[
            m_activeWindow
        ];

    if (m_selectedButton < 0 ||
        m_selectedButton >=
        static_cast<int>(
            window.buttons.size()
            ))
    {
        return false;
    }

    const GameButtonRect& button =
        window.buttons[
            m_selectedButton
        ];

    x =
        button.x +
        button.width * 0.5f;

    y =
        button.y +
        button.height * 0.5f;

    return true;
}

void MM3GameModule::selectButtonInDirection(
    int dx,
    int dy
)
{
    if (m_activeWindow < 0 ||
        m_activeWindow >=
        static_cast<int>(
            m_windows.size()
            ))
    {
        return;
    }

    MM3Window& window =
        m_windows[
            m_activeWindow
        ];

    if (window.buttons.empty())
    {
        return;
    }

    if (m_selectedButton < 0 ||
        m_selectedButton >=
        static_cast<int>(
            window.buttons.size()
            ))
    {
        m_selectedButton =
            0;
    }

    const GameButtonRect& currentButton =
        window.buttons[
            m_selectedButton
        ];

    const float currentCenterX =
        currentButton.x +
        currentButton.width * 0.5f;

    const float currentCenterY =
        currentButton.y +
        currentButton.height * 0.5f;

    int bestButton =
        -1;

    float bestDistance =
        FLT_MAX;

    for (int i = 0;
        i < static_cast<int>(
            window.buttons.size()
            );
        ++i)
    {
        if (i == m_selectedButton)
        {
            continue;
        }

        const GameButtonRect& button =
            window.buttons[i];

        const float centerX =
            button.x +
            button.width * 0.5f;

        const float centerY =
            button.y +
            button.height * 0.5f;

        const float deltaX =
            centerX -
            currentCenterX;

        const float deltaY =
            centerY -
            currentCenterY;

        if (std::abs(deltaX) < 5.0f &&
            std::abs(deltaY) < 5.0f)
        {
            continue;
        }

        if (dx < 0)
        {
            if (deltaX >= 0.0f ||
                -deltaX <= std::abs(deltaY))
            {
                continue;
            }
        }

        if (dx > 0)
        {
            if (deltaX <= 0.0f ||
                deltaX <= std::abs(deltaY))
            {
                continue;
            }
        }

        if (dy < 0)
        {
            if (deltaY >= 0.0f ||
                -deltaY <= std::abs(deltaX))
            {
                continue;
            }
        }

        if (dy > 0)
        {
            if (deltaY <= 0.0f ||
                deltaY <= std::abs(deltaX))
            {
                continue;
            }
        }

        float distance = 0.0f;

        if (dy != 0)
        {
            // Up / Down:
            // gleiche Spalte stark bevorzugen
            distance =
                deltaY * deltaY +
                deltaX * deltaX * 10.0f;
        }
        else
        {
            // Left / Right:
            // gleiche Zeile stark bevorzugen
            distance =
                deltaX * deltaX +
                deltaY * deltaY * 10.0f;
        }

        if (distance <
            bestDistance)
        {
            bestDistance =
                distance;

            bestButton =
                i;
        }
    }

    if (bestButton >= 0)
    {
        m_selectedButton =
            bestButton;

        m_pendingMousePosition =
            true;
    }


}

bool MM3GameModule::
blockDirectDosBoxKeyboard() const
{
    return
        m_buttonMode ||
        m_activeWindow == 0;
}

void MM3GameModule::updateActiveButton(
    const GameButtonRect& rect
)
{
    if (m_activeWindow < 0 ||
        m_activeWindow >=
        static_cast<int>(
            m_windows.size()
            ))
    {
        return;
    }

    MM3Window& window =
        m_windows[
            m_activeWindow
        ];

    if (m_selectedButton < 0 ||
        m_selectedButton >=
        static_cast<int>(
            window.buttons.size()
            ))
    {
        return;
    }

    window.buttons[
        m_selectedButton
    ] =
        rect;

        saveButtons();
}

void MM3GameModule::deleteActiveButton()
{
    if (m_activeWindow < 0 ||
        m_activeWindow >=
        static_cast<int>(
            m_windows.size()
            ))
    {
        return;
    }

    MM3Window& window =
        m_windows[
            m_activeWindow
        ];

    if (m_selectedButton < 0 ||
        m_selectedButton >=
        static_cast<int>(
            window.buttons.size()
            ))
    {
        return;
    }

    window.buttons.erase(
        window.buttons.begin() +
        m_selectedButton
    );

    if (window.buttons.empty())
    {
        m_selectedButton =
            -1;
    }
    else if (m_selectedButton >=
        static_cast<int>(
            window.buttons.size()
            ))
    {
        m_selectedButton =
            static_cast<int>(
                window.buttons.size()
                ) - 1;
    }

    saveButtons();
}


    void MM3GameModule::update()
    {
        m_stateReader.update(
            m_state
        );

        int detectedWindow =
            0;

        if (mainMenuVisible())
        {
            detectedWindow =
                1;
        }
        else if (loadGameVisible())
        {
            detectedWindow =
                2;
        }

        else if (matchesScreen(
            MightAndMagic3::
            ScreenSignatures::
            mainGame()
        ))
        {
            detectedWindow =
                3;
        }
        // Index 3 bleibt vorläufig deaktiviert.

        const bool windowChanged =
            m_activeWindow !=
            detectedWindow;

        m_activeWindow =
            detectedWindow;

        if (!windowChanged)
        {
            return;
        }

        if (m_activeWindow <= 0 ||
            m_activeWindow >=
            static_cast<int>(
                m_windows.size()
                ))
        {
            m_selectedButton =
                -1;

            return;
        }

        const MM3Window& window =
            m_windows[
                m_activeWindow
            ];

        m_selectedButton =
            window.buttons.empty()
            ? -1
            : 0;

        if (m_selectedButton >= 0)
        {
            m_pendingMousePosition =
                true;
        }
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
    GameButtonRect& rect
) const
{
    if (!m_buttonMode)
    {
        rect = {};
        return false;
    }

    if (m_activeWindow < 0 ||
        m_activeWindow >=
        static_cast<int>(
            m_windows.size()
            ))
    {
        rect = {};
        return false;
    }

    const MM3Window& window =
        m_windows[
            m_activeWindow
        ];

    if (m_selectedButton < 0 ||
        m_selectedButton >=
        static_cast<int>(
            window.buttons.size()
            ))
    {
        rect = {};
        return false;
    }

    rect =
        window.buttons[
            m_selectedButton
        ];

    return true;
}

void MM3GameModule::activateSelectedButton()
{
    if (m_activeWindow < 0 ||
        m_activeWindow >=
        static_cast<int>(
            m_windows.size()
            ))
    {
        return;
    }

    const MM3Window& window =
        m_windows[
            m_activeWindow
        ];

    if (m_selectedButton < 0 ||
        m_selectedButton >=
        static_cast<int>(
            window.buttons.size()
            ))
    {
        return;
    }

    m_pendingMouseClick =
        true;
}

bool MM3GameModule::mainMenuVisible() const
{
    return matchesScreen(
        MightAndMagic3::
        ScreenSignatures::
        mainMenu()
    );
}

bool MM3GameModule::loadGameVisible() const
{
    return matchesScreen(
        MightAndMagic3::
        ScreenSignatures::
        loadGame()
    );
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
        if (matches(
            MightAndMagic3::KeyAction::
            Cancel
        ))
        {
            m_pendingDosKey =
                "ESC";

            return;
        }
    }

    if (matches(
        MightAndMagic3::KeyAction::
        ActivateButton
    ))
    {
        if (m_buttonMode &&
            m_activeWindow >= 0)
        {
            activateSelectedButton();

            return;
        }

        m_pendingDosKey =
            "ENTER";

        return;
    }

    if (m_buttonMode &&
        m_activeWindow >= 0)
    {
        MM3Window& window =
            m_windows[
                m_activeWindow
            ];

        if (window.buttons.empty())
        {
            return;
        }

        if (matches(
            MightAndMagic3::KeyAction::
            ButtonUp
        ))
        {
            selectButtonInDirection(
                0,
                -1
            );

            return;
        }

        if (matches(
            MightAndMagic3::KeyAction::
            ButtonDown
        ))
        {
            selectButtonInDirection(
                0,
                1
            );

            return;
        }

        if (matches(
            MightAndMagic3::KeyAction::
            ButtonLeft
        ))
        {
            selectButtonInDirection(
                -1,
                0
            );

            return;
        }

        if (matches(
            MightAndMagic3::KeyAction::
            ButtonRight
        ))
        {
            selectButtonInDirection(
                1,
                0
            );

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

void MM3GameModule::keyUp(
    SDL_Keycode key
)
{}