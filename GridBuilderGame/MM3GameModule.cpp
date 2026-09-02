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
        MightAndMagic3::KeyAction::TogglePortraitMode,
        static_cast<int>(SDLK_DELETE)
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

    MM3Window characterScreenWindow;

    characterScreenWindow.name =
        "Character Screen";

    m_windows.push_back(
        characterScreenWindow
    );

    MM3Window inventoryWindow;

    inventoryWindow.name =
        "Inventory";

    m_windows.push_back(
        inventoryWindow
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
    if (m_portraitMode)
    {
        if (m_portraitControls.size() <
            PortraitControlCount)
        {
            m_portraitControls.push_back(
                rect
            );

            m_selectedPortrait =
                static_cast<int>(
                    m_portraitControls.size()
                    ) - 1;
        }

        saveButtons();

        return;
    }

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

    file
        << "portraits"
        << '\n';

    for (const GameButtonRect& button :
        m_portraitControls)
    {
        file
            << "portrait "
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
        else if (type == "portrait")
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

            m_portraitControls.push_back(
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
    if (m_pendingInventorySlot0Click)
    {
        m_pendingInventorySlot0Click =
            false;

		x = 85.0f 
            /* X von Item Slot 0 */;

		y = 52.0f 
            /* Y von Item Slot 0 */;

        return true;
    }

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

    if (m_portraitMode)
    {
        if (m_selectedPortrait < 0 ||
            m_selectedPortrait >=
            static_cast<int>(
                m_portraitControls.size()
                ) ||
            !portraitControlAvailable(
                m_selectedPortrait
            ))
        {
            return false;
        }

        const GameButtonRect& button =
            m_portraitControls[
                m_selectedPortrait
            ];

        x =
            button.x +
            button.width * 0.5f;

        y =
            button.y +
            button.height * 0.5f;

        return true;
    }

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
        16.0f;

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

    if (m_portraitMode)
    {
        if (m_selectedPortrait < 0 ||
            m_selectedPortrait >=
            static_cast<int>(
                m_portraitControls.size()
                ))
        {
            return false;
        }

        const GameButtonRect& button =
            m_portraitControls[
                m_selectedPortrait
            ];

        x =
            button.x +
            button.width * 0.5f;

        y =
            button.y +
            button.height * 0.5f;

        return true;
    }

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

bool MM3GameModule::portraitControlAvailable(
    int index
) const
{
    if (index < 0 ||
        index >= PortraitControlCount)
    {
        return false;
    }

    if (index <= 5)
    {
        return true;
    }

    if (index == 6)
    {
        return
            m_stateReader.characterLevel(6) >
            0;
    }

    if (index == 7)
    {
        return
            m_stateReader.characterLevel(7) >
            0;
    }

    // Index 8 = Einstellungen
    return true;
}

bool MM3GameModule::
blockDirectDosBoxKeyboard() const
{
    return
        m_buttonMode ||
        m_portraitMode;
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
            characterScreen()
        ))
        {
            detectedWindow =
                4;
        }

        else if (matchesScreen(
            MightAndMagic3::
            ScreenSignatures::
            inventory()
        ))
        {
            detectedWindow =
                5;
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

        if (windowChanged &&
            m_activeWindow == 5)
        {
            m_pendingInventorySlot0Click =
                true;
        }

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
        if (m_portraitMode)
        {
            if (m_selectedPortrait < 0 ||
                m_selectedPortrait >=
                static_cast<int>(
                    m_portraitControls.size()
                    ))
            {
                rect = {};
                return false;
            }

            rect =
                m_portraitControls[
                    m_selectedPortrait
                ];

            return true;
        }

        if (!m_buttonMode)
        {
            rect = {};
            return false;
        }
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

    // -------------------------------------------------
    // Button Mode ein/aus
    // -------------------------------------------------

    if (matches(
        MightAndMagic3::KeyAction::
        ToggleButtonMode
    ))
    {
        m_buttonMode =
            !m_buttonMode;

        if (m_buttonMode)
        {
            m_portraitMode =
                false;
        }

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

    // -------------------------------------------------
    // Portrait Mode ein/aus
    // -------------------------------------------------

    if (matches(
        MightAndMagic3::KeyAction::
        TogglePortraitMode
    ))
    {
        m_portraitMode =
            !m_portraitMode;

        if (m_portraitMode)
        {
            m_buttonMode =
                false;

            if (!m_portraitControls.empty())
            {
                m_selectedPortrait =
                    0;

                m_pendingMousePosition =
                    true;
            }
            else
            {
                m_selectedPortrait =
                    -1;
            }
        }

        return;
    }

    // -------------------------------------------------
    // Portrait Mode
    // -------------------------------------------------

    if (m_portraitMode)
    {
        // ESC

        if (matches(
            MightAndMagic3::KeyAction::
            Cancel
        ))
        {
            m_pendingDosKey =
                "ESC";

            return;
        }

        // Links

        if (matches(
            MightAndMagic3::KeyAction::
            TurnLeft
        ))
        {
            int next =
                m_selectedPortrait - 1;

            while (next >= 0 &&
                !portraitControlAvailable(
                    next
                ))
            {
                --next;
            }

            if (next >= 0)
            {
                m_selectedPortrait =
                    next;

                m_pendingMousePosition =
                    true;
            }

            return;
        }

        // Rechts

        if (matches(
            MightAndMagic3::KeyAction::
            TurnRight
        ))
        {
            int next =
                m_selectedPortrait + 1;

            // 0-7 = Portraits
            // 8 = Einstellungen

            while (next < 8 &&
                !portraitControlAvailable(
                    next
                ))
            {
                ++next;
            }

            if (next < 8)
            {
                m_selectedPortrait =
                    next;

                m_pendingMousePosition =
                    true;
            }

            return;
        }

        // Up -> Einstellungen

        if (matches(
            MightAndMagic3::KeyAction::
            Forward
        ))
        {
            if (m_selectedPortrait >= 0 &&
                m_selectedPortrait < 8 &&
                m_portraitControls.size() > 8)
            {
                m_lastSelectedPortrait =
                    m_selectedPortrait;

                m_selectedPortrait =
                    8;

                m_pendingMousePosition =
                    true;
            }

            return;
        }

        // Down -> letztes Portrait

        if (matches(
            MightAndMagic3::KeyAction::
            Backward
        ))
        {
            if (m_selectedPortrait == 8)
            {
                m_selectedPortrait =
                    m_lastSelectedPortrait;

                m_pendingMousePosition =
                    true;
            }

            return;
        }

        // Return -> Portrait / Einstellungen klicken

        if (matches(
            MightAndMagic3::KeyAction::
            ActivateButton
        ))
        {
            if (m_selectedPortrait >= 0 &&
                m_selectedPortrait <
                static_cast<int>(
                    m_portraitControls.size()
                    ) &&
                portraitControlAvailable(
                    m_selectedPortrait
                ))
            {
                m_pendingMouseClick =
                    true;
            }

            return;
        }

        // Im Portrait Mode keine Bewegung
        // an DOSBox weitergeben.

        return;
    }

    // -------------------------------------------------
    // ESC funktioniert auch außerhalb der Modi
    // -------------------------------------------------

    if (matches(
        MightAndMagic3::KeyAction::
        Cancel
    ))
    {
        m_pendingDosKey =
            "ESC";

        return;
    }

    // -------------------------------------------------
    // Normaler Button Mode
    // -------------------------------------------------

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

        // Return

        if (matches(
            MightAndMagic3::KeyAction::
            ActivateButton
        ))
        {
            activateSelectedButton();

            return;
        }

        // Up

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

        // Down

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

        // Links

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

        // Rechts

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

        // Im Button Mode keine Bewegung
        // an DOSBox weitergeben.

        return;
    }

    // -------------------------------------------------
    // Game Mode
    // -------------------------------------------------

    if (matches(
        MightAndMagic3::KeyAction::
        ActivateButton
    ))
    {
        m_pendingDosKey =
            "ENTER";

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