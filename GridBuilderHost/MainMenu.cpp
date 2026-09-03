#include "MainMenu.h"

#include "imgui.h"

namespace GridBuilderHost
{
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
}