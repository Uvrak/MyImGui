#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>
#include "../GridBuilderCore/GameModule.h"

namespace DosBoxX
{
    class FrameTexture;
    class Mouse;
    class NamedPipeClient;
}

namespace MyImGui
{
    class SettingsWindow;
}

namespace GridBuilderHost
{
    class MainMenu;
    class DosBoxWindow;
    class HostFontSettings;

    class HostUi
    {
    public:
        HostUi(
            MainMenu& mainMenu,
            DosBoxWindow& dosBoxWindow,
            MyImGui::SettingsWindow& settingsWindow,
            HostFontSettings& hostFontSettings
        );

        void draw(
            DosBoxX::FrameTexture& frameTexture,
            uint32_t contentWidth,
            uint32_t contentHeight,
            DosBoxX::Mouse& dosBoxMouse,
            DosBoxX::NamedPipeClient& dosBoxPipeClient,
            const std::vector<GameButtonRect>& buttonRects,
            std::optional<GameButtonRect> selectedButton,
            std::optional<GameButtonRect> selectedInventory,
            bool buttonEditingActive,
            bool buttonViewAvailable,
            const std::string& buttonViewName
        );
        std::optional<GameButtonRect> takeDrawnButton();
        std::optional<std::size_t> takeDeletedButton();
        std::optional<GameButtonEdit> takeModifiedButton();
        void setButtonSaveFailed(bool failed) { m_buttonSaveFailed = failed; }
        std::optional<GameButtonPoint> dosBoxScreenPosition(GameButtonPoint point) const;
        std::optional<GameButtonPoint> onLeftMouseButtonDown(
            float mouseX,
            float mouseY,
            DosBoxX::Mouse& dosBoxMouse,
            DosBoxX::NamedPipeClient& dosBoxPipeClient
        );

    private:
        MainMenu& m_mainMenu;
        DosBoxWindow& m_dosBoxWindow;

        bool m_showDosBoxView =
            true;
        bool m_buttonSaveFailed = false;

        MyImGui::SettingsWindow& m_settingsWindow;
        HostFontSettings& m_hostFontSettings;
    };
}
