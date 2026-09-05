#include "HostUi.h"

#include "MainMenu.h"
#include "DosBoxWindow.h"
#include "SettingsWindow.h"
#include "HostFontSettings.h"
#include "Mouse.h"
#include "NamedPipeClient.h"

namespace GridBuilderHost
{
    HostUi::HostUi(
        MainMenu& mainMenu,
        DosBoxWindow& dosBoxWindow,
        MyImGui::SettingsWindow& settingsWindow,
        HostFontSettings& hostFontSettings
    )
        :
        m_mainMenu(
            mainMenu
        ),
        m_dosBoxWindow(
            dosBoxWindow
        ),
        m_settingsWindow(
            settingsWindow
        ),
        m_hostFontSettings(
            hostFontSettings
        )
    {}

    void HostUi::draw(
        DosBoxX::FrameTexture& frameTexture,
        uint32_t contentWidth,
        uint32_t contentHeight,
        DosBoxX::Mouse& dosBoxMouse,
        DosBoxX::NamedPipeClient& dosBoxPipeClient
    )
    {
        m_hostFontSettings.update();

        m_mainMenu.draw();

        if (m_mainMenu.consumeOpenSettingsRequest())
        {
            m_settingsWindow.open();
        }

        if (m_settingsWindow.isOpen())
        {
            m_settingsWindow.draw();
        }

        if (m_showDosBoxView)
        {
            m_dosBoxWindow.draw(
                frameTexture,
                contentWidth,
                contentHeight,
                m_mainMenu.showDosBoxCoordinates(),
                dosBoxMouse,
                dosBoxPipeClient
            );
        }
    }
}