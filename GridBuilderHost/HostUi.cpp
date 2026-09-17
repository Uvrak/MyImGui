#include "HostUi.h"

#include "MainMenu.h"
#include "DosBoxWindow.h"
#include "SettingsWindow.h"
#include "HostFontSettings.h"
#include "Mouse.h"
#include "NamedPipeClient.h"

namespace GridBuilderHost
{
    std::optional<GameButtonPoint> HostUi::dosBoxScreenPosition(GameButtonPoint point) const
    {
        return m_showDosBoxView ? m_dosBoxWindow.screenPosition(point) : std::nullopt;
    }

    std::optional<GameButtonPoint> HostUi::onLeftMouseButtonDown(
        float mouseX,
        float mouseY,
        DosBoxX::Mouse& dosBoxMouse,
        DosBoxX::NamedPipeClient& dosBoxPipeClient
    )
    {
        if (m_showDosBoxView)
            return m_dosBoxWindow.onLeftMouseButtonDown(
                mouseX, mouseY, dosBoxMouse, dosBoxPipeClient);
        return std::nullopt;
    }

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

    std::optional<GameButtonRect> HostUi::takeDrawnButton()
    {
        return m_dosBoxWindow.takeDrawnButton();
    }

    std::optional<std::size_t> HostUi::takeDeletedButton()
    {
        return m_dosBoxWindow.takeDeletedButton();
    }

    std::optional<GameButtonEdit> HostUi::takeModifiedButton()
    {
        return m_dosBoxWindow.takeModifiedButton();
    }

    void HostUi::draw(
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
                dosBoxPipeClient,
                buttonRects,
                selectedButton,
                selectedInventory,
                buttonEditingActive,
                buttonViewAvailable,
                buttonViewName,
                m_buttonSaveFailed
            );
        }
    }
}
