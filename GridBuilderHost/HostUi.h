#pragma once

#include <cstdint>

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
            DosBoxX::NamedPipeClient& dosBoxPipeClient
        );

    private:
        MainMenu& m_mainMenu;
        DosBoxWindow& m_dosBoxWindow;

        bool m_showDosBoxView =
            true;

        MyImGui::SettingsWindow& m_settingsWindow;
        HostFontSettings& m_hostFontSettings;
    };
}