#include "MainMenu.h"
#include "HostSettings.h"

#include "imgui.h"

namespace GridBuilderHost
{
    MainMenu::MainMenu(
        HostSettings& hostSettings
    )
        :
        m_hostSettings(
            hostSettings
        ),
        m_showDosBoxCoordinates(
            hostSettings.showDosBoxCoordinates()
        )
    {}

    void MainMenu::draw()
    {
        if (!ImGui::BeginMainMenuBar())
        {
            return;
        }

        if (ImGui::BeginMenu(
            "Settings"
        ))
        {
            if (ImGui::MenuItem(
                "Appearance..."
            ))
            {
                m_openSettingsRequested =
                    true;
            }

            ImGui::Separator();

            if (ImGui::MenuItem(
                "DOSBox Coordinates",
                nullptr,
                &m_showDosBoxCoordinates
            ))
            {
                m_hostSettings.setShowDosBoxCoordinates(
                    m_showDosBoxCoordinates
                );

                m_hostSettings.save();
            }

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    bool GridBuilderHost::MainMenu::consumeOpenSettingsRequest()
    {
        if (!m_openSettingsRequested)
        {
            return false;
        }

        m_openSettingsRequested =
            false;

        return true;
    }
    bool MainMenu::showDosBoxCoordinates() const
    {
        return
            m_showDosBoxCoordinates;
    }
}