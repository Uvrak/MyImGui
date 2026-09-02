#pragma once
#include "GameModule.h"
#include "GameState.h"
#include "MM3KeyBindings.h"
#include "ScreenSignatures.h"


#include <string>
#include <vector>
#include <array>
#include <windows.h>

namespace DosBoxX
{
    struct DosBoxFrameHeader;
}

namespace DosBoxX
{
    class Controller;
    class NamedPipeClient;
}

namespace MightAndMagic3
{
    class StateReader;
}

struct MM3Window
{
    std::string name;

    std::vector<GameButtonRect>
        buttons;
};

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
        GameButtonRect& rect
    ) const override;

    void addButtonToActiveWindow(
        const GameButtonRect& rect
    ) override;

    bool m_pendingInventorySlot0Click =
        false;

    void saveButtons() const;

    void loadButtons();

    void updateActiveButton(
        const GameButtonRect& rect
    ) override;

    void deleteActiveButton() override;

    bool takeDosMouseClick(
        float& x,
        float& y
    ) override;


    bool takeDosMouseDoubleClick(
        float& x,
        float& y
    ) override;

    bool takeDosMousePosition(
        float& x,
        float& y
    ) override;

    struct PortraitRect
    {
        float x;
        float y;
        float width;
        float height;
    };

private:

    void selectButtonInDirection(
        int dx,
        int dy
    );

    bool portraitControlAvailable(
        int index
    ) const;

    int m_selectedPortrait = 0;

    bool m_buttonMode = true;

    bool m_portraitMode = false;

    int m_lastSelectedPortrait = 0;

    std::vector<GameButtonRect>
        m_portraitControls;

    static constexpr int PortraitCount = 9;
    static constexpr int PortraitControlCount = 9;


    MightAndMagic3::StateReader&
        m_stateReader;

    MightAndMagic3::GameState
        m_state;

    MightAndMagic3::KeyBindings
        m_keyBindings;

    std::string m_pendingDosKey;

    int m_selectedButton =
        -1;

    std::vector<MM3Window>
        m_windows;

    int m_activeWindow =
        -1;

    void activateSelectedButton();

    bool m_pendingMouseClick =
        false;

    const DosBoxX::DosBoxFrameHeader*
        m_frameHeader =
        nullptr;

    const uint8_t*
        m_framePixels =
        nullptr;

    bool mainMenuVisible() const;

    bool loadGameVisible() const;

    bool m_pendingMouseDoubleClick =
        false;

    bool m_pendingMousePosition =
        false;

    std::array<
        PortraitRect,
        PortraitCount
    > m_portraits;
};

    
